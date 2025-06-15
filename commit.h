//
// Created by dylan on 6/15/2025.
//

#ifndef COMMIT_H
#define COMMIT_H


#include <string>
#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>

#include "util.h"

using namespace std;

const string NORMAL_FILE_MODE = "100644";

string buildCommitTree(vector<IndexEntry>&);
void buildCommitObject(string, string&);
void commit(string&);


#endif //COMMIT_H
