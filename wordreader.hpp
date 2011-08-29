#ifndef WORDREADER_H
#define WORDREADER_H

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

struct TargetWord {
	std::string word;
	int id;
	int x;
	int y;
	int height;
	int width;
};

struct Sentence{
	int id;
	std::vector<TargetWord> words;
}

class WordReader {
	
	private:
		// Vector of sentence for each player
		std::vector< std::vector<Sentence> > sentences;
		
	public:
		WordReader(int playerCount);
		~WordReader();
		
		// Returns null if no such word
		TargetWord getWord(int player, int sentenceNumber, int wordNumber);
};

#endif
