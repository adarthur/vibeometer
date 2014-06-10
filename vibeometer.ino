
/*
   Arduino Vibeometer
  
 This sketch controls a device that interacts with Twitter to display the campus mood at NC State University. 
 See go.ncsu.edu/arduino or github.com/adarthur/vibeometer for more information.
  
 created 1 November 2013
 modified 22 May 2014
 by Aaron Arthur
 MIT License
*/

#include <WiFi.h>
#include <avr/wdt.h>

#define soft_reset() do { wdt_enable(WDTO_15MS); for(;;) { } } while (0)
#define ONE_DAY 86400000 // 24 hrs in milliseconds. For soft reset.

#define MAXFREQ 5.0 // highest frequency of the LED "heartbeat" (Hz)
#define NUMLEDS 3 // number of LED panels to update
#define GOODPIN 9 // pin for the "happy tweet" indicator
#define BADPIN 6 // pin for the "unhappy tweet" indicator
#define MENTIONPIN 5 // pin for the "thanks for the mention" indicator

long currentMillis;

// pins for shift register
const int latchPin = 8;
const int clockPin = 3;
const int dataPin = 2;

// arrays for LED information
const int ledPins[NUMLEDS] = {GOODPIN, BADPIN, MENTIONPIN}; // put the pins in an array for easy access
double ledFreqs[NUMLEDS]; // frequencies at which LEDs are to blink
double tempFreqs[NUMLEDS]; // temporary storage for frequency values

//tweet stuff
int numMentions = 0; // number of mentions (displayed on 7-seg)
int lastMentions = 0; // a temp variable for the above
int tweetCount[3]; // counters for each type of tweet; format is {good tweets, bad tweets, mentions}
int lastGoodTweets = 0; // temp variable for tweetCount[0]
int lastBadTweets = 0; // temp variable for tweetCount[1]
int mentioned[NUMLEDS]; // is there a new mention? order is good mention, bad mention, regular mention
int lastMentioned[NUMLEDS]; // temp variable for mentioned[]

char ssid[] = "your ssid"; // network SSID (ncsu doesn't require a password but does require registration with NOMAD
char pass[] = "your password";
String currentLine = ""; // holder for parsing output from web page
String getString = "your getTweets.php"; // URL to GET (location of PHP page)

int status = WL_IDLE_STATUS;
char server[] = "your server"; // name address for your server (without 'http://')

WiFiClient client;

void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3"))); // to redisable the watch-dog timer after reset

void setup() {
  wdt_init(); // turn the wdt off so we don't get continual resets
  
  // initialize LED stiff
  int i;

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  digitalWrite(latchPin, HIGH);
  
  pinMode(A5, INPUT);

  for(i = 0; i < NUMLEDS; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
    ledFreqs[i] = 0;
    mentioned[i] = 0;
    tweetCount[i] = 0;
  }

  // Initialize serial and wait for port to open:
  Serial.begin(9600); 
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  //initialize the counter display to "00"
  updateCounter(0);

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  } 

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) { 
    //Serial.print("Attempting to connect to SSID: ");
    //Serial.println(ssid);
    status = WiFi.begin(ssid); // Connect to an open network. Change this line if using WPA or WEP network:  

    // wait 5 seconds for connection:
    delay(5000);
  } 

  // Connected (probably); check serial output to be sure
  Serial.println("Connected to wifi");
  printWifiStatus();
}

void loop() {
  currentMillis = millis();
  
  updateLEDs();
  connectToServer();
  parseWebPage();
  updateLEDs();
  updateMoodFreqs();
  outputToPanels();
  
  if(millis() > ONE_DAY) soft_reset();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// display a decimal number, x, on the 2-digit 7-segment counter
void updateCounter(int x) {
  byte highNibble = x / 10; //tens digit
  byte lowNibble = x % 10; //ones digit
  byte toSend = (lowNibble << 4) + highNibble; //BCD forms (ones first) of count digits

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, toSend);
  digitalWrite(latchPin, HIGH);
}

// update LED brightnesses to follow sinusoids
void updateLEDs() {
  int nowMillis = millis();
  for(int i = 0; i <= 2; i++) {
    analogWrite(ledPins[i],255*sin(PI*ledFreqs[i]*nowMillis/1000)*sin(PI*ledFreqs[i]*nowMillis/1000));
  }
}

void connectToServer() {
  //Serial.println("\nStarting connection to server...");
  updateLEDs();
  if (client.connect(server, 80)) {
    updateLEDs();
    //Serial.print(millis());
    //updateLEDs();
    //Serial.println("connected to server");
    //updateLEDs();
    // Make a HTTP request:
    client.print("GET ");
    updateLEDs();
    client.print(getString);
    updateLEDs();
    client.println(" HTTP/1.1");
    updateLEDs();
    client.print("Host:");
    updateLEDs();
    client.println(server);
    updateLEDs();
    client.println("Connection: close");
    updateLEDs();
    client.println();
    updateLEDs();
  }

  while(!client.available()){
    updateLEDs();
  }
}

// parse the text output by the PHP page
void parseWebPage() {
	String currentLine = "";
  char charBuff[10];
  int i = 0;	
	
  if(client.find("\r\n\r\n")){ // skip the header of the message
    while (client.available()) {               // the data we're reading is in the format "g:b:m_nnn"
      char c = client.read();                  // g is the numberof good tweets, b the number of bad tweets,
      if(c == ':') {                           // and m the number of mentions. These influence the mood panels.
        currentLine.toCharArray(charBuff, 10); // n1, n2, and n3 are 1 if there is a new good tweet mentioning @vibeometer,
        tweetCount[i] = atoi(charBuff);        // bad tweet mentioning it, or just a new mention, respectively.
        currentLine = "";
        i++;
      }
      else if(c == '_') {
        currentLine.toCharArray(charBuff, 10);
        tweetCount[i] = atoi(charBuff);
        for(i = 0; i < 3; i++) {
          mentioned[i] = client.read();
        }
      }
      else currentLine += c;      
      updateLEDs();
    }
	
	client.flush(); //get rid of anything in the buffer
  }
	
	// translate ASCII to decimal
  for(i = 0; i < 3; i++) {
    if(mentioned[i] == 48) mentioned[i] = 0;
    if(mentioned[i] == 49) mentioned[i] = 1;
    updateLEDs();
  }
	
	numMentions = tweetCount[2];
}

// set the frequencies of the flashing of the good and bad mood panels
void updateMoodFreqs() {
  double totalTweets = tweetCount[0] + tweetCount[1];
  
  if(tweetCount[0] > 0) ledFreqs[0] = MAXFREQ * (tweetCount[0] / totalTweets);
  else ledFreqs[0] = 1; // if there's no Twitter data coming in or no tweets, flash at 1 Hz
  
  if(tweetCount[1] > 0) ledFreqs[1] = MAXFREQ * (tweetCount[1] / totalTweets); 
  else ledFreqs[1] = 1;
}

// handle any output to the flashing panels
void outputToPanels() {
	// store the frequency values so they can be restored later if the frequency is changed below
  tempFreqs[0] = ledFreqs[0];
  tempFreqs[1] = ledFreqs[1];
  
  // if there is a new good tweet, flash the good mood panel at 8 Hz,
  // but only one time
  if(mentioned[0] && mentioned[0] != lastMentioned[0]) {
    ledFreqs[0] = 8;
  }
	
  // same thing for a bad tweet
  if(mentioned[1] && mentioned[0] != lastMentioned[0]) {
    ledFreqs[1] = 8;
  }

  lastMentioned[0] = mentioned[0];
  lastMentioned[1] = mentioned[1];
	
  // flash the "thanks for the @mention" panel if there is a new mention of @vibeometer
  if(numMentions > lastMentions) {
    ledFreqs[2] = MAXFREQ;
  }
  else ledFreqs[2] = 0;
  
  if(numMentions >= 100) numMentions = 99; // make sure the count never goes above two digits
  lastMentions = numMentions;  
  updateCounter(numMentions);

  // let the mention panel flash for 3 seconds
  for(currentMillis = millis(); millis() - currentMillis < 3000; ) { 
    updateLEDs();
  }
  
  ledFreqs[2] = 0; // stop the flashing mention panel
  
  // let the flashing mood panel(s) go for 1.5 more seconds
  for(currentMillis = millis(); millis() - currentMillis < 1500; ) { 
    updateLEDs();
  }

  // let the mood panels flash at the proper frequency again
  ledFreqs[0] = tempFreqs[0];
  ledFreqs[1] = tempFreqs[1];
  
  // wait 6.5 more seconds before starting again
  for(currentMillis = millis(); millis() - currentMillis < 6500; ) { 
    updateLEDs();
  }
}

void wdt_init(void)
{
    MCUSR = 0;
    wdt_disable();

    return;
}
