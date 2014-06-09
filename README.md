vibeometer
==========

Arduino Vibeometer: a Twitter interface to show the NC State campus mood.

Check out http://go.ncsu.edu/arduino for some documentation. 

The wrapper for connecting to Twitter is from J7mbo here: https://github.com/J7mbo/twitter-api-php

To get started with this using your own Arduino and your own account, you'll need to change a few things in the code.
In getTweets.php:

*Put the appropriate tokens in the $settings array at the top of the file. To get access tokens for your app, go to apps.twitter.com

In vibeometer.ino:

*Change global variables ssid and pass (if applicable) to work with your network. Be sure to change the line "status = WiFi.begin(ssid)" to fit this; this line is toward the end of setup(). See the page at http://arduino.cc/en/Guide/ArduinoWiFiShield#toc6 for more examples of this.

*Change getString to the URL of your getTweets.php, minus the server, e.g. /www/getTweets.php

*Change server to the URL of your server, e.g. "http://www.myserver.com/"

*To change the delay between polling getTweets.php, modify the for() loops at the end of outputToPanels(). Note that with the current Twitter authentication scheme, searches are rate-limited to 180 per 15 minutes. For more info see https://dev.twitter.com/docs/rate-limiting/1.1

To modify what the page searches for on Twitter, I recommend testing your queries first using the API console here: https://apigee.com/console/twitter
