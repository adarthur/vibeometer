<?php

	/**
		Get Tweets: a page polled by the Vibeometer box (an Arduino)
			  Author: Aaron Arthur
			 License: MIT License
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

	$search_string = "%28ncsu%29";
	$good_search = "+AND+%28good+OR+great+OR+happy+OR+better+OR+lucky+OR+best%29";
	$bad_search = "+AND+&28sad+OR+bad+OR+annoyed+OR+grumpy+OR+sick+OR+angry+OR+mad%29";


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

				$count = array(0,0,0); //number of tweets this day in each category: good, bad, @vibeometer
	$newTweet = array(0,0,0); //1 or 0, is there a new tweet this minute?

	for($i = 0; $i < 3; $i++) {
		$obj = json_decode($json[$i], true);
		$j = 0;
		do {
			$tweetText = $obj["statuses"][$j]["text"];
			$tweetTime = strtotime($obj["statuses"][$j]["created_at"]); //incrementally get the timestamp of each tweet in the category
			$nowTime = time();
						
			if($nowTime - $tweetTime < 60) $newTweet[$i] = 1; //tell the box to flash if there's a new good tweet in the last minute
			
			if($nowTime - $tweetTime < 2 * 60 * 60 && $i != 2) { //count good/bad tweets from the last two hours
				$count[$i]++; 
			}
			
			if($nowTime - $tweetTime < 7 * 24 * 60 * 60 && $i == 2) //count mentions from the last 7 days
			{
				$count[$i]++;
				if($nowTime - $tweeTime < 2 * 60) //analyse and/or respond to a mention from the last minute
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
	
	function respond_to_tweet($twitter, $tweet_obj) {
		$screen_name = $tweet_obj["user"]["screen_name"];
		$subject = $tweet_obj["text"];
		$test_pattern = '/test/';
		
		if(preg_match($test_pattern, $subject)) {
			update_status($twitter, "@" . $screen_name . " responding!");
		}
		
		else return;
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
