#ifndef WORDREADER_H
#define WORDREADER_H

#include <vector>
#include <string>

struct TargetWord {
	int id;
	std::string word;
	double width;
	double distance;
};

class Sentence{
	public:
		Sentence(int number): id(number){}
		int id;
		std::vector<TargetWord> words;
		void add(TargetWord word){
			words.push_back(word);
		}
		size_t size(){
			return words.size();
		}
		TargetWord operator[](int pos) const{
			return words[pos];
		}
};

class WordReader {
	
	private:
		// Vector of sentence for each player
		std::vector< std::vector<Sentence> > sentences;
		int _playerCount;
		
	public:
		WordReader(int playerCount);
		~WordReader();
		
		// Returns null if no such word
		TargetWord getWord(int player, int sentenceNumber, int wordNumber);
		size_t maxSentence()
		{ return sentences[0].size(); }
		size_t sentenceLength(int sentenceId)
		{ return sentences[0][sentenceId-1].size(); }
};

#endif
