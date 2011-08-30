#include "wordreader.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstring>

using std::string;
using std::vector;
using std::ifstream;

WordReader::WordReader(int playerCount) : _playerCount(playerCount) {
	sentences.resize(playerCount);
	std::cout << "initializing wordreader" << std::endl;
	for(int i = 1; i <= playerCount; i++){
		std::stringstream ss;
		ss << "targetwords." << i << ".txt";
		std::cout << "looking for file " << ss.str() << std::endl;
		ifstream file(ss.str().c_str());
		if(file.is_open()){
			string line;
			int sentencenumber = 1;
			int wordnumber = 0;
			Sentence sentence(1);
			while(file.good()){
				getline(file, line);
				if(line.length() < 3) {
					continue;
				}
				if(line.find_first_of("#") != string::npos){
					continue;
				}
				char * cstr = new char [line.size()+1];
				strcpy (cstr, line.c_str());
				const char * part = strtok(cstr, ";");
				
				if(part != NULL){
					if(atoi(part) > sentencenumber){
						// Save sentence
						sentences[i-1].push_back(sentence);
						sentencenumber = atoi(part);
						sentence = Sentence(sentencenumber);
					}
				}
				else{
					std::cout << "Incomplete line in file targetwords." << i << ".txt" << " sentence " << sentencenumber << std::endl;
				}
				part = strtok(NULL, ";");
				if(part != NULL){
					wordnumber = atoi(part);
				}
				else{
					std::cout << "Incomplete line in file targetwords." << i << ".txt" << " sentence " << sentencenumber << std::endl;
				}
				part = strtok(NULL, ";");
				string word;
				if(part != NULL){
					word = part;
				}
				else{
					std::cout << "Incomplete line in file targetwords." << i << ".txt" << " sentence " << sentencenumber << std::endl;
				}
				int width;
				part = strtok(NULL, ";");
				if(part != NULL){
					width = atoi(part);
				}
				else{
					std::cout << "Incomplete line in file targetwords." << i << ".txt" << " sentence " << sentencenumber << std::endl;
				}
				int x;
				part = strtok(NULL, ";");
				if(part != NULL){
					x = atoi(part);
				}
				else{
					std::cout << "Incomplete line in file targetwords." << i << ".txt" << " sentence " << sentencenumber << std::endl;
				}
				int y;
				part = strtok(NULL, ";");
				if(part != NULL){
					y = atoi(part);
				}
				else{
					std::cout << "Incomplete line in file targetwords." << i << ".txt" << " sentence " << sentencenumber << std::endl;
				}

				TargetWord tw = {wordnumber, word, x, y, width, width};
				sentence.add(tw);
				std::cout << "read word: " << tw.word << " x: " << tw.x << " y: " << tw.y << std::endl;
			}
			file.close();
			sentences[i-1].push_back(sentence);
		}
		else{
			std::cout << "file not found" << std::endl;
		}
	}
	std::cout << "sentences size: " << sentences.size();
	std::cout << " sentences[0] size: " << sentences[0].size();
	std::cout << " sentences[0][0] size: " << sentences[0][0].size();
	std::cout << "sentences[0][0][0]" << sentences[0][0][0].word << std::endl;
};

WordReader::~WordReader(){}
		
// Returns null if no such word
TargetWord WordReader::getWord(int player, int sentenceNumber, int wordNumber){
	return sentences[player][sentenceNumber-1][wordNumber-1];
}
