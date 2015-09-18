#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <random>
#include "common.h"
#include "patterns.h"
#include "../libs/asmlib.h"

using namespace std;

void Patterns::initialize() {
	this->initializePatterns();
}

void Patterns::initializePatterns() {
	stringstream ss;
	ss << "patterns-" << this->textFileName << "-" << this->m << "-"  << this->queriesNum << ".dat";
	string s = ss.str();
	char *patternFileName = (char *)(s.c_str());
	unsigned int textLen, queriesFirstIndexArrayLen;
	unsigned char *text = readFileChar(this->textFileName, textLen, 0);
	unsigned int *queriesFirstIndexArray;

	if (!fileExists(patternFileName)) {
		cout << "Generating " << this->queriesNum << " patterns of length " << this->m << " from " << this->textFileName;
		if (this->ordCharsLen != 0) {
			cout << ", alphabet (ordinal): {";
			for (unsigned int i = 0; i < this->ordCharsLen; ++i) {
				cout << this->ordChars[i];
				if ((i + 1) != this->ordCharsLen) {
					cout << ", ";
				}
			}
			cout << "}";
		}
		cout << " ... " << flush;

		random_device rd;
		mt19937 gen(rd());
		uniform_int_distribution<unsigned int> dis(100, textLen - m - 101);

		queriesFirstIndexArray = new unsigned int[this->queriesNum];

		unsigned int genVal;

		if (this->ordCharsLen != 0) {
			for (unsigned long long i = 0; i < this->queriesNum; ++i) {
				genVal = dis(gen);
				queriesFirstIndexArray[i] = genVal;
				for (unsigned int j = 0; j < this->m; ++j) {
					bool inSigma = false;
					for (unsigned int k = 0; k < this->ordCharsLen; ++k) {
						if (text[j + genVal] == this->ordChars[k]) {
							inSigma = true;
							break;
						}
					}
					if (!inSigma) {
						--i;
						break;
					}
				}
			}
		} else {
			for (unsigned int i = 0; i < this->queriesNum; ++i) {
				genVal = dis(gen);
				if ((int)text[genVal + m - 1] == 255) {
					--i;
					continue;
				}
				queriesFirstIndexArray[i] = genVal;
			}
		}
		cout << "Done" << endl;
		cout << "Saving patterns in " << patternFileName << " ... " << flush;
		FILE* outFile;
		outFile = fopen(patternFileName, "w");
		fwrite(queriesFirstIndexArray, (size_t)4, (size_t)(this->queriesNum), outFile);
		fclose(outFile);
		cout << "Done" << endl;
	} else {
		cout << "Reading patterns from " << patternFileName << " ... " << flush;
		queriesFirstIndexArray = readFileInt(patternFileName, queriesFirstIndexArrayLen, 0);
		cout << "Done" << endl;
	}
	this->patterns = new unsigned char *[this->queriesNum];
	for (unsigned int i = 0; i < queriesNum; ++i) {
		this->patterns[i] = new unsigned char[this->m + 1];
		this->patterns[i][this->m] = '\0';
		for (unsigned int j = 0; j < this->m; ++j) {
			this->patterns[i][j] = text[queriesFirstIndexArray[i] + j];
		}
	}

	delete[] queriesFirstIndexArray;
	delete[] text;
}

void Patterns::initializeSACounts() {
	stringstream ss;
	ss << "counts-" << this->textFileName << "-" << this->m << "-"  << this->queriesNum << ".dat";
	string s = ss.str();
	char *countsFileName = (char *)(s.c_str());

	if (!fileExists(countsFileName)) {
		unsigned int textLen;
		unsigned char *text = readFileChar(this->textFileName, textLen, 0);

		unsigned int saLen;
		unsigned int *sa = readSA(this->textFileName, saLen, 0, false);

		this->counts = new unsigned int[this->queriesNum];

		for (unsigned int i = 0; i < this->queriesNum; ++i) {
			this->counts[i] = this->getSACount(sa, text, saLen, this->patterns[i], this->m);
		}

		FILE* outFile;
		outFile = fopen(countsFileName, "w");
		fwrite(this->counts, (size_t)4, (size_t)(this->queriesNum), outFile);
		fclose(outFile);

		delete[] text;
		delete[] sa;

	} else {
		unsigned int countsLen;
		this->counts = readFileInt(countsFileName, countsLen, 0);
	}
}

void Patterns::freeMemory() {
	for (unsigned int i = 0; i < this->queriesNum; ++i) {
		delete[] this->patterns[i];
	}
	delete[] this->patterns;
	delete[] this->counts;
	delete[] this->ordChars;
}


unsigned int Patterns::getSACount(unsigned int *sa, unsigned char *text, unsigned int saLen, unsigned char *pattern, int patternLength) {
	unsigned int beg = 0, end = 0;
	binarySearch(sa, text, 0, saLen, pattern, patternLength, beg, end);
	return end - beg;
}

void Patterns::binarySearch(unsigned int *sa, unsigned char *text, unsigned int lStart, unsigned int rStart, unsigned char *pattern, int patternLength, unsigned int &beg, unsigned int &end) {
	unsigned int l = lStart;
	unsigned int r = rStart;
	unsigned int mid;
	while (l < r) {
		mid = (l + r) / 2;
		if (A_strcmp((const char*)pattern, (const char*)(text + sa[mid])) > 0) {
			l = mid + 1;
		}
		else {
			r = mid;
		}
	}
	beg = l;
	r = rStart;
	pattern[patternLength - 1]++;
	while (l < r) {
		mid = (l + r) / 2;
		if (A_strcmp((const char*)pattern, (const char*)(text + sa[mid])) <= 0) {
			r = mid;
		}
		else {
			l = mid + 1;
		}
	}
	end = r;
}

unsigned char **Patterns::getPatterns() {
	return this->patterns;
}

unsigned int *Patterns::getSACounts() {
	if (this->counts == NULL) this->initializeSACounts();
	return this->counts;
}

void Patterns::setSelectedChars(string selectedChars) {
	if (selectedChars != "all") this->ordChars = breakByDelimeter(selectedChars, ',', this->ordCharsLen);
}

unsigned int Patterns::getErrorCountsNumber(unsigned int *countsToCheck) {
	if (this->counts == NULL) this->initializeSACounts();
	unsigned int errorCountsNumber = 0;
	for (unsigned int i = 0; i < this->queriesNum; ++i) {
		if (countsToCheck[i] != this->counts[i]) ++errorCountsNumber;
	}
	return errorCountsNumber;
}