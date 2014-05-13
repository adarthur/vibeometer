
/*
 Arduino Vibeometer
  
 This sketch controls a device that interacts with Twitter to display the campus mood at NC State University. 
  
 created 13 July 2010
 modified 13 May 2014
 by Aaron Arthur
*/


#include <SPI.h>
#include <WiFi.h>

#define NUMLEDS 3
#define MAXFREQ 5.0 //highest frequency of the LED "heartbeat" (Hz)


#define GOODPIN 9 //pin for the "happy tweet" indicator
#define BADPIN 6 //pin for the "unhappy tweet" indicator
#define MENTIONPIN 5 //pin for the "thanks for the mention" indicator


//pins for shift register (for 7-segments)
const int latchPin = 8;
const int clockPin = 3;
const int dataPin = 2;

const int ledPins[NUMLEDS] = {GOODPIN, BADPIN, MENTIONPIN}; //put the pins in an array for easy access
double ledFreqs[NUMLEDS]; //frequencies at which LEDs are to blink
int mentioned[NUMLEDS]; //did something happen on Twitter? order is good, bad, mentioned
int numMentions = 0; //number of mentions (displayed on 7-seg)
int lastMentions = 0; 
double tweetCount[3]; //counters for each type of tweet

char ssid[] = "ncsu"; //  your network SSID (name) 
String currentLine = "";
String getString = "/display/techshowcase/getTweets.php"; //URL to GET

int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(152,14,136,38);  // numeric IP for NCSU Libraries (no DNS)
char server[] = "webdev.lib.ncsu.edu";    // name address for NCSU Libraries (using DNS)

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

  if(client.find("\r\n\r\n")){  
    while (client.available()) {
      char c = client.read();
      if(c == ':') {
        currentLine.toCharArray(charBuff, 10);        
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

    client.flush();
  }

  updateLEDs();
  for(i = 0; i < 3; i++) {
    if(mentioned[i] == 48) mentioned[i] = 0;
    if(mentioned[i] == 49) mentioned[i] = 1;
    updateLEDs();
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    updateLEDs();
  }
  
  double totalTweets = tweetCount[0] + tweetCount[1];
  ledFreqs[0] = MAXFREQ * (tweetCount[0] / totalTweets);
  ledFreqs[1] = MAXFREQ * (tweetCount[1] / totalTweets);
  
  //ledFreqs[0] = 5*analogRead(A5)/1023 + 1; //for testing with potentiometer

  //handle outputs regarding mentions @vibeometer
  numMentions = tweetCount[2];

  if(numMentions > lastMentions) ledFreqs[2] = MAXFREQ;
  else ledFreqs[2] = 0;
  
  if(numMentions >= 100) numMentions = 99;
  
  lastMentions = numMentions;
  updateCounter(numMentions);

  for(currentMillis = millis(); millis() - currentMillis < 3000; ) { 
    updateLEDs();
  }
  
  ledFreqs[2] = 0;
  
  for(currentMillis = millis(); millis() - currentMillis < 9000; ) { 
    updateLEDs();
  }
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

void updateCounter(int x) {
  byte highNibble = x / 10; //tens digit
  byte lowNibble = x % 10; //ones digit
  byte toSend = (lowNibble << 4) + highNibble; //BCD forms (ones first) of count digits

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, toSend);
  digitalWrite(latchPin, HIGH);
}

void updateLEDs() {
  int nowMillis = millis();
  for(int i = 0; i <= 2; i++) {
    analogWrite(ledPins[i],255*sin(PI*ledFreqs[i]*nowMillis/1000)*sin(PI*ledFreqs[i]*nowMillis/1000));
  }
  return;
}
