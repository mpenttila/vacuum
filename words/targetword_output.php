<?php

if(count($argv) < 3) exit("Wrong number of arguments\n");

$infilename = trim($argv[1]);
$outfilename = trim($argv[2]);

//$ifile = fopen($infilename, 'r');
$ofile = fopen($outfilename, 'w');

$contents = file_get_contents($infilename);
$contents = str_replace("\n", " ", $contents);

$sentences = array();
$sid = 1;
$wid = 1;
$wordcount = 0;

$word = strtok($contents, " \n\t");
while($word !== false && $wordcount < 136){
	if(strcasecmp($word,'a') == 0 || strcasecmp($word,'an') == 0 || strcasecmp($word,'the') == 0){
		if(($word2 = strtok(" \n\t")) !== false){
			$word = $word.' '.$word2;
		}
	}
	$sentences[$sid][$wid] = $word;
	$wid++;
	if(strpos($word, ".") !== false){
		$sid++;
		$wid = 1;
	}
	$word = strtok(" \n\t");
	$wordcount++;
}

$w = array(150, 70, 15);
$d = array(900, 1350, 1800);
$wi = 0;
$di = 0;
$wc = 0;
$dc = 0;

echo "writing to file ".$outfilename."\n";

foreach($sentences as $i => $sentence){
	foreach($sentence as $j => $word){
		//echo $i.';'.$j.';'.$word.';'.$w[$wi].';'.$d[$di]."\n";
		fwrite($ofile, $i.';'.$j.';'.$word.';'.$w[$wi].';'.$d[$di]."\n");
		$wc++;
		$dc++;
		if($dc == 15){
			$di++;
			$dc = 0;
		}
		if($di == 3) $di = 0;
		if($wc == 45){
			$wi++;
			$wc = 0;
		}
		if($wi == 3) break;
	}
}
fclose($ofile);
