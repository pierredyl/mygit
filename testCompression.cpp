//
// Created by dylan on 6/15/2025.
//


#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "util.h"

using namespace std;


int main() {

    string data = "Testing with compression and decompression";

    vector<unsigned char> rawData(data.begin(), data.end());

    cout << "Raw data: " << endl;
    for (int i = 0; i < rawData.size(); i++) {
        cout << rawData[i];
    }
    cout << "\n\n";

    vector<unsigned char> compressedData = compressUsingDeflate(rawData);

    cout << "Compressed data: " << endl;
    for (int i = 0; i < compressedData.size(); i++) {
        cout << compressedData[i];
    }

    cout << "\n\n";

    vector<unsigned char> decompressedData = decompressUsingInflate(compressedData);

    cout << "Decompressed data: " << endl;
    for (int i = 0; i < decompressedData.size(); i++) {
        cout << decompressedData[i];
    }
    cout << "\n\n";


    cout << "Testing reading file data into a char vector" << endl;
    ifstream stream("blob3", ios::binary);

    //istreambuf_iterator doesn't allow unsigned chars, so first read it as chars
    vector<char> fileData{ istreambuf_iterator<char>(stream), istreambuf_iterator<char>()};

    //Then convert to unsigned char
    vector<unsigned char> fileDataUnsigned(fileData.begin(), fileData.end());

    stream.close();

    cout << "File data raw: " << endl;
    for (int i = 0; i < fileDataUnsigned.size(); i++) {
        cout << fileDataUnsigned[i];
    }
    cout << "\n\n";

    vector<unsigned char> compressedFileData = compressUsingDeflate(fileDataUnsigned);
    cout << "Compressed file data: " << endl;
    for (int i = 0; i < compressedFileData.size(); i++) {
        cout << compressedFileData[i];
    }
    cout << "\n\n";

    vector<unsigned char> decompressedFileData = decompressUsingInflate(compressedFileData);
    cout << "Decompressed file data: " << endl;
    for (int i = 0; i < decompressedFileData.size(); i++) {
        cout << decompressedFileData[i];
    }
    cout << "\n\n";
    return 0;
}

