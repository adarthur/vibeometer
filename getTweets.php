<?php

    ini_set('display_errors', 1);
	
    date_default_timezone_set('UTC');
	$tweetCount = 100;

    require_once('TwitterAPIExchange.php');

     

     /** Set access tokens here - see: https://dev.twitter.com/apps/ **/

    $settings = array(

        'oauth_access_token' => "1964944952-H3UgxKwMvOlD9knBUh9LbEvSe6yvIqdvqc1yMka",

        'oauth_access_token_secret' => "4njJgLZPTZYfgvt4v3PZzWPesR3ILH2BSUwlG7BZy1ioA",

        'consumer_key' => "F3EEOFz3RU8izg3g4Gyk8A",

        'consumer_secret' => "sUyeTfc8UKOIDtFQxL9BrvgTecZbyJFNLg7OHkplhH8"

    );

     

    /** Perform a GET request and echo the response **/

    $url = 'https://api.twitter.com/1.1/search/tweets.json';
	
    $search_string = "%28ncsu%29";
    $good_search = "+AND+%28good+OR+great+OR+happy+OR+better+OR+lucky+OR+best%29";
    //$bad_search = "+AND+%28sad+OR+mad+OR+bored+OR+tired+OR+bad+OR+worst%29";
	//$good_search = "+AND+%28happy+excited+pleased+thrilled+hopeful+joyful%29";
	$bad_search = "+AND+&28sad+annoyed+grumpy+sick+angry+mad%29";


    $good_getfield = "?q=" . $search_string . $good_search . "&count=" . $tweetCount;
    $bad_getfield = "?q=" . $search_string . $bad_search . "&count=" . $tweetCount;
    $ncsulib_getfield = "?q=%40vibeometer" . "&count=" . $tweetCount;
    $requestMethod = 'GET';

    $twitter = new TwitterAPIExchange($settings);

    $json = array($twitter->setGetfield($good_getfield)

                    ->buildOauth($url, $requestMethod)

                    ->performRequest(),

    		  $twitter->setGetfield($bad_getfield)

                    ->buildOauth($url, $requestMethod)

                    ->performRequest(),

			$twitter->setGetfield($ncsulib_getfield)

                    ->buildOauth($url, $requestMethod)

                    ->performRequest(),);


	$count = array(0,0,0); //number of tweets this day in each category: good, bad, @ncsulibraries
	$newTweet = array(0,0,0); //1 or 0, is there a new tweet this minute?
	
	for($i = 0; $i < 3; $i++) {
		$obj = json_decode($json[$i], true);
		$j = 0;
		do {
			$tweetTimeStamp = $obj["statuses"][$j]["created_at"]; //incrementally get the timestamp of each tweet in the category
			$explodedTimeStamp = explode(" ", $tweetTimeStamp); //isolate them into an array
			$tweetDate = $explodedTimeStamp[2]; //give the date of the tweet, e.g. "05"
			$explodedTime = explode(":", $explodedTimeStamp[3]);
			$tweetHour = $explodedTime[0];
			$tweetMinute = $explodedTime[1];
			$tweetSecond = $explodedTime[2];
			//echo "$tweetTimeStamp " . date('d H:i') . "<br />"; //for debugging time comparison
			if($tweetDate == date('d') && $tweetHour == date('H') && $tweetMinute == date('i')) $newTweet[$i] = 1;
			if($tweetDate == date('d') && date('H') - $tweetHour < 2 && $i != 2) $count[$i]++; //count good/bad tweets from the last hour
			if(date('d') - $tweetDate < 7 && $i == 2) 
			{
				$count[$i]++; //count mentions from the last day
				if($tweetDate == date('d') && $tweetHour == date('H') && date('i') - $tweetMinute < 2)
				{
					if(is_good_tweet($obj["statuses"][$j]["text"])) $newTweet[0] = 1;
					if(is_bad_tweet($obj["statuses"][$j]["text"])) $newTweet[1] = 1;
				}
			}
			$j++; //move on to the next tweet in the category
			//keep going until 30 or if the timestamp doesn't exist anymore (i.e., you run out of tweets)
		} while ($tweetDate > 0 && $j < $tweetCount); 
	}
	
	//the below output is in the following format
	//goodCount:badCount:@mentionCount_newGoodnewBadnew@mention
	//parse on Arduino accordingly
	
    for($i = 0; $i < 3; $i++) {
		echo "$count[$i]";
		if($i < 2) echo ":";
    }
	echo "_";
	for($i = 0; $i < 3; $i++) {
		echo "$newTweet[$i]";
    }
	
	function is_good_tweet($tweet_text) {
		$subject = $tweet_text;
		$good_pattern = '/good|great|happy|better|lucky|best/';
		
		if(preg_match($good_pattern, $subject)) return true;
		else return false;
	}
	
	function is_bad_tweet($tweet_text) {
		$subject = $tweet_text;
		$bad_pattern = '/sad|annoyed|grumpy|sick|angry|mad/';
		
		if(preg_match($bad_pattern, $subject)) return true;
		else return false;
	}
?>