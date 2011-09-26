#ifndef DISTANTLOGGER_H
#define DISTANTLOGGER_H
 
#include <stdio.h>
#include <time.h>
#include <string>
#include <iostream>
#include <fstream>
#include "boost/date_time/posix_time/posix_time.hpp"

#define LOG_FILE "logs/"
#define VACUUM_MODE 0
#define DISTORT_MODE 1

using namespace std;
using namespace boost::posix_time;

class Logger {
	private:
		time_t startTime;
		struct tm * localStartTime;
		ptime startMicroTime;
		ofstream logstream;
		int gameNumber;
	
	public:
		Logger(int mode);
		~Logger();
		
		void startRound();
		void logAcquire(int player, int sentenceid, int wordid, string word, float width, float distance);
		void endRound();
		void endGame(int scores[], int players);
	
};


#endif
