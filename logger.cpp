#include <algorithm>
#include "logger.hpp"

Logger::Logger() : gameNumber(0) {
	time(&startTime);
	localStartTime = localtime(&startTime);
	char date[50];
	strftime(date, sizeof(date), "%Y%m%d_%H%M", localStartTime);
	const string logfilename(string(LOG_FILE) + string(date) + ".log");
	logstream.open(logfilename.c_str());
	if(logstream.is_open()){
		logstream << "Vacuum expirement log file created on " << asctime (localStartTime) << endl;
		logstream << "player,sentence id,word id,word,width (mm),distance (mm),acquire time (ms)" << endl;
	}
}

Logger::~Logger(){
	logstream.close();
}


void Logger::startRound(){
	gameNumber++;
	time(&startTime);
	localStartTime = localtime(&startTime);
	ptime t(microsec_clock::local_time());
	startMicroTime = t;
	/*
	wstring timeString = strToWstr(to_simple_string(startMicroTime));
	logstream << timeString << endl;
	logstream << "ROUND " << gameNumber << ": New game started in mode: " << wideMode << endl;
	logstream << "ROUND " << gameNumber << ": Player 1: " << _player1Name << ", Player 2: " << _player2Name << endl;
	*/
}

void Logger::logAcquire(int player, int sentenceid, int wordid, string word, float width, float distance){
	ptime acquireTime(microsec_clock::local_time());
	time_duration difference = acquireTime - startMicroTime;
	// Note player + 1, i.e. left player is 0 in code -> player 1 in log
	logstream << player+1 << "," << sentenceid << "," << wordid << "," << word << "," << width << "," << distance << "," << difference.total_milliseconds() << endl;
}

void Logger::endRound(){
	logstream.flush();
}

void Logger::endGame(int scores[], int players){
	time(&startTime);
	localStartTime = localtime(&startTime);
	logstream << "Experiment ended on " << asctime (localStartTime) << endl;
	logstream <<  "Final score: ";
	for(int i = 0; i < players; i++){
		logstream << "Player " << i+1 << ": " << scores[i] << " ";
	}
	logstream << endl;
	logstream.flush();
}
