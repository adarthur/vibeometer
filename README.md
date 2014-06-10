Vibeometer
==========

Arduino Vibeometer: a Twitter interface to show the NC State campus mood.

Check out http://go.ncsu.edu/arduino for more information. 

The wrapper for connecting to Twitter is from J7mbo here: https://github.com/J7mbo/twitter-api-php

To get started with this using your own Arduino and your own account, you'll need to change a few things in the code.

In getTweets.php:

* You must put the appropriate tokens in the $settings array at the top of the file. To get access tokens for your app, go to apps.twitter.com

* You can change the search queries by changing the variables `$search_string`, `$good_search`, etc. I recommend testing the queries first using the API console here: https://apigee.com/console/twitter

In vibeometer.ino:

* You must change global variables ssid and pass (if applicable) to work with your network. Be sure to change the line `status = WiFi.begin(ssid)` accordingly; this line is toward the end of `setup()`. See the page at http://arduino.cc/en/Guide/ArduinoWiFiShield#toc6 for more examples of this.

* You must change getString to the URL of your getTweets.php, minus the server, e.g. /www/getTweets.php

* You must change server to the URL of your server, e.g. "http://www.myserver.com/"

* You can change the delay between polling getTweets.php by modifying the `for()` loops at the end of `outputToPanels()`. Note that with the current Twitter authentication scheme, searches are rate-limited to 180 per 15 minutes. For more info see https://dev.twitter.com/docs/rate-limiting/1.1
