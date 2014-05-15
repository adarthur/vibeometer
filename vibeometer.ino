/*
 Arduino Vibeometer
  
 This sketch controls a device that interacts with Twitter to display the campus mood at NC State University. 
  
 created 13 July 2010
 modified 13 May 2014
 by Aaron Arthur
*/

#include <SPI.h>
#include <WiFi.h>

#define NUMLEDS 3 //number of LED panels to update
#define MAXFREQ 5.0 //highest frequency of the LED "heartbeat" (Hz)


#define GOODPIN 9 //pin for the "happy tweet" indicator
#define BADPIN 6 //pin for the "unhappy tweet" indicator
#define MENTIONPIN 5 //pin for the "thanks for the mention" indicator

#define UPDATE_MOOD_FREQ(X) (18*sin(PI*24*(X))+1) // When someone tweets @vibeometer with a mood word, the frequency of that mood panel changes according to this (see the end of loop)

//pins for shift register
const int latchPin = 8;
const int clockPin = 3;
const int dataPin = 2;

const int ledPins[NUMLEDS] = {GOODPIN, BADPIN, MENTIONPIN}; //put the pins in an array for easy access
double ledFreqs[NUMLEDS]; //frequencies at which LEDs are to blink
double tempFreqs[NUMLEDS]; //temporary storage for frequency values
int mentioned[NUMLEDS]; //did something happen on Twitter? order is good, bad, mentioned
int numMentions = 0; //number of mentions (displayed on 7-seg)
int lastMentions = 0; 
int tweetCount[3]; //counters for each type of tweet
int lastGoodTweets = 0;
int lastBadTweets = 0;

char ssid[] = "ncsu"; //network SSID (ncsu doesn't require a password but does require registration with NOMAD
String currentLine = ""; //holder for parsing output from 
String getString = "[your URL here]"; //URL to GET (location of PHP page)

int status = WL_IDLE_STATUS;
//if you don't want to use DNS (and reduce your sketch size)
//use the numeric IP instead of the name for the server:
char server[] = "[your server here]";    //name address for your server

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

void setup() {
  //initialize LED stiff
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

  //Initialize serial and wait for port to open:
  Serial.begin(9600); 
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  //initialize the counter to 0
  updateCounter(0);

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  } 

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) { 
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:  
    // Nope, for us it's an open network (NCSU Nomad)  
    status = WiFi.begin(ssid);

    // wait 5 seconds for connection:
    delay(5000);
  } 

  //Hooray, we're connected (probably; check serial output to be sure
  Serial.println("Connected to wifi");
  printWifiStatus();
}

void loop() {
  int i, j, k;
  long currentMillis = millis();
  int freqTest;
  updateLEDs();
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    updateLEDs();
    Serial.print(millis());
    updateLEDs();
    Serial.println("connected to server");
    updateLEDs();
    // Make a HTTP request:
    client.print("GET ");
    updateLEDs();
    client.print("/display/techshowcase/getTweets.php");
    updateLEDs();
    client.println(" HTTP/1.1");
    updateLEDs();
    client.println("Host:webdev.lib.ncsu.edu");
    updateLEDs();
    client.println("Connection: close");
    updateLEDs();
    client.println();
    updateLEDs();
  }

  //Wait for incoming bits, and then put the 0s and 1s into currentLine
  while(!client.available()){
    updateLEDs();
  }

  String currentLine = "";
  i = 0;
  char charBuff[10];

  if(client.find("\r\n\r\n")){ //skip the header of the message
    while (client.available()) {               // the data we're reading is in the format "g:b:m_xxx"
      char c = client.read();    q             // the last three numbers are deprecated, but g is the number
      if(c == ':') {                           // of good tweets, b the number of bad tweets, and m the number
        currentLine.toCharArray(charBuff, 10); // of mentions       
        tweetCount[i] = atoi(charBuff);
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

  updateLEDs();
  
  //translate ASCII to decimal (deprecated feature)
  for(i = 0; i < 3; i++) {
    if(mentioned[i] == 48) mentioned[i] = 0;
    if(mentioned[i] == 49) mentioned[i] = 1;
    updateLEDs();
  }

  // if the server's disconnected, stop the client
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    updateLEDs();
  }
  
  //set the frequencies of the flashing of the good and bad mood panels
  double totalTweets = tweetCount[0] + tweetCount[1];
  ledFreqs[0] = MAXFREQ * (tweetCount[0] / totalTweets);
  ledFreqs[1] = MAXFREQ * (tweetCount[1] / totalTweets); 
  
  //handle outputs regarding mentions @vibeometer
  numMentions = tweetCount[2];
  
  //flash the "thanks for the @mention" panel if there is a new mention of @vibeometer
  if(numMentions > lastMentions) ledFreqs[2] = MAXFREQ;
  else ledFreqs[2] = 0;
  
  //store the frequency values so they can be restored later if the frequency is changed below
  tempFreqs[0] = ledFreqs[0];
  tempFreqs[1] = ledFreqs[1];
  //if there is a good tweet @vibeometer, flash the good mood panel at twice the MAX
  if(numMentions > lastMentions && tweetCount[0] > lastGoodTweets) ledFreqs[0] = 2*MAXFREQ;
  //same thing for a bad tweet @vibeometer
  if(numMentions > lastMentions && tweetCount[1] > lastBadTweets) ledFreqs[1] = 2*MAXFREQ;
  
  if(numMentions >= 100) numMentions = 99; // make sure the count never goes above two digits
  
  lastMentions = numMentions;
  lastGoodTweets = twetCount[0];
  lastBadTweets = tweetCount[1];
  
  updateCounter(numMentions);

  //let the mention panel flash for 3 seconds
  for(currentMillis = millis(); millis() - currentMillis < 3000; ) { 
    updateLEDs();
  }
  
  ledFreqs[2] = 0;
  
  //wait 9 more seconds before starting again
  for(currentMillis = millis(); millis() - currentMillis < 9000; ) { 
    updateLEDs();
  }
  
  //let the mood panels flash at the proper frequency again
  ledFreqs[0] = tempFreqs[0];
  ledFreqs[1] = tempFreqs[1];
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

//Send some decimal number to the 2-digit 7-segment counter
void updateCounter(int x) {
  byte highNibble = x / 10; //tens digit
  byte lowNibble = x % 10; //ones digit
  byte toSend = (lowNibble << 4) + highNibble; //BCD forms (ones first) of count digits

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, toSend);
  digitalWrite(latchPin, HIGH);
}

//Update LED brightness to follow a sinusoid
void updateLEDs() {
  int nowMillis = millis();
  for(int i = 0; i <= 2; i++) {
    analogWrite(ledPins[i],255*sin(PI*ledFreqs[i]*nowMillis/1000)*sin(PI*ledFreqs[i]*nowMillis/1000));
  }
}
