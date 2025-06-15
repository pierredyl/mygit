//
// Created by dylan on 6/15/2025.
//

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <zlib.h>
#include <openssl/sha.h>

using namespace std;

struct IndexEntry {
    string hashString;
    string path;
    vector<unsigned char> hashBinary;
};

constexpr int CHUNK_SIZE = 16384;



string readFile(const string&);
unsigned char hexCharToNum(char);
vector<unsigned char> hashStringToBinary(string);
string sha256(string);
vector<unsigned char> compressUsingDeflate(vector<unsigned char>);
vector<unsigned char> decompressUsingInflate(vector<unsigned char>);
void writeBinaryToFile(const string&, vector<unsigned char>&);
pair<string, string> parseConfigFileForUser();
string parseHeadForBranch(ifstream&);
vector<IndexEntry> collectAllIndexEntries();




#endif //UTIL_H
