<?php

	/**
		Get Tweets: a page polled by the Vibeometer box (an Arduino)
			  Author: Aaron Arthur
			 License: MIT License
			 Website: http://www.lib.ncsu.edu/vibeometer
	**/

	ini_set('display_errors', 1);

	date_default_timezone_set('America/New_York');

	$tweetCount = 100;

	require_once('TwitterAPIExchange.php');


	 /** Set access tokens here - see: https://apps.twitter.com/ **/

	$settings = array(

			'oauth_access_token' => "your access token",

			'oauth_access_token_secret' => "your access token secret",

			'consumer_key' => "your consumer key",

			'consumer_secret' => "your consumer key secret"

	);

	 

	/** Perform a GET request and echo the response **/

	$url = 'https://api.twitter.com/1.1/search/tweets.json';

	$search_string = "(ncsu)%20AND%20";
	$good_search = "(good%20OR%20great%20OR%20happy%20OR%20better%20OR%20lucky%20OR%20best)";
	$bad_search = "(sad%20OR%20bad%20OR%20fail%20OR%20annoyed%20OR%20sick%20OR%20angry%20OR%20mad)";


	$good_getfield = "?q=" . $search_string . $good_search . "&count=" . $tweetCount;
	$bad_getfield = "?q=" . $search_string . $bad_search . "&count=" . $tweetCount;
	$mention_getfield = "?q=%40vibeometer" . "&count=" . $tweetCount;
	$requestMethod = 'GET';

	$twitter = new TwitterAPIExchange($settings);

	$json = array($twitter->setGetfield($good_getfield)

												->buildOauth($url, $requestMethod)

												->performRequest(),

								$twitter->setGetfield($bad_getfield)

												->buildOauth($url, $requestMethod)

												->performRequest(),

								$twitter->setGetfield($mention_getfield)

												->buildOauth($url, $requestMethod)

												->performRequest(),);


	$count = array(0,0,0); //number of tweets in each category: good, bad, @vibeometer
	$newTweet = array(0,0,0); //1 or 0, is there a new tweet this minute?
	
	for($i = 0; $i < 3; $i++) {
		$obj = json_decode($json[$i], true);
		$j = 0;
		do {
			$tweetText = $obj["statuses"][$j]["text"];
			$tweetTime = strtotime($obj["statuses"][$j]["created_at"]); //incrementally get the timestamp of each tweet in the category
			$nowTime = time();
			$timeDiff = $nowTime - $tweetTime;
			
			$isReTweet = preg_match('/RT/', $tweetText);
						
			if($timeDiff < 60) $newTweet[$i] = 1; //tell the box to flash if there's a new good tweet in the last minute
			
			if($timeDiff < 24 * 60 * 60 && $i != 2 && !$isReTweet) { //count good/bad tweets from the last 24 hours
				$count[$i]++; 
			}
			
			if($timeDiff < 7 * 24 * 60 * 60 && $i == 2) //count mentions from the last 7 days
			{
				$count[$i]++;
				if($timeDiff < 90) //analyse and/or respond to a mention from the last minute
				{
					if(is_good_tweet($obj["statuses"][$j]["text"])) $newTweet[0] = 1;
					if(is_bad_tweet($obj["statuses"][$j]["text"])) $newTweet[1] = 1;
					respond_to_tweet($twitter, $obj["statuses"][$j]);
				}
			}
			$j++; //move on to the next tweet in the category
			
		} while ($tweetTime > 0 && $j < $tweetCount); //keep going until 100 or if the timestamp doesn't exist anymore (i.e., you run out of tweets)
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
	
	// currently, these functions are only invoked if there is a mention
	function is_good_tweet($tweet_text) {
		$subject = $tweet_text;
		$good_pattern = '/good|best|great|happy|cool|awesome|sweet/';
		
		if(preg_match($good_pattern, $subject)) return true;
		else return false;
	}
	
	function is_bad_tweet($tweet_text) {
		$subject = $tweet_text;
		$bad_pattern = '/bad|sad|annoyed|grumpy|sick|angry|mad/';
		
		if(preg_match($bad_pattern, $subject)) return true;
		else return false;
	}
	
	function respond_to_tweet($twitter, $tweet_obj, $count) {
		$screen_name = $tweet_obj["user"]["screen_name"];
		$subject = $tweet_obj["text"];
		$test_pattern = '/test/';
		$greet_pattern = '/hi|hello|greetings|what[\']*s up/';
		$link_pattern = '/t\.co/';
		$what_pattern = '/what happens/';
		$user_feeling = '/I.*feel|I[\']*m feeling/';
		$mood_query = '/mood|campus/';
		
		if(preg_match($test_pattern, $subject)) $response = " Responding!";
		
		else if(preg_match($what_pattern, $subject)) $response = " This happens!"; 
		
		else if(preg_match($greet_pattern, $subject)) $response = " I deal in vibes, not in greetings. How are you feeling today?";
		
		else if(preg_match($user_feeling, $subject)) {
			if(is_good_tweet($subject) && is_bad_tweet($subject)) $response = " I'm picking up some mixed vibes from you.";
			else if(is_good_tweet($subject)) $response = " Glad to hear it!";
			else if(is_bad_tweet($subject)) $response = " that's too bad; maybe you'll feel better if you mention me again!";
			else $response = " I don't think I'm qualified to diagnose that..";
		}
		
		else if(preg_match($mood_query, $subject)) {
			$response = " I'm feeling great! The vibe on campus is";
			if($count[0] > $count[1]) $response = $response . " not so good...";
			else $response = $response . " great, too!";
		}
		
		else if(preg_match($link_pattern, $subject))	$response = " I'm not sure where that link goes, but I hope it's a picture of me!";
		
		else $response = " You rang?";
				
		if(!preg_match('/vibeometer/', $screen_name)) update_status($twitter, "@" . $screen_name . $response);
	}
	
	function update_status($twitter, $status_text) {
		$post_url = 'https://api.twitter.com/1.1/statuses/update.json';
		$post_request = 'POST';
		
		$array = array('status' => $status_text);
		$twitter->getfield = NULL;
	
		$twitter->setPostfields($array)
					  ->buildOauth($post_url, $post_request)
					  ->performRequest();
	}
?>
