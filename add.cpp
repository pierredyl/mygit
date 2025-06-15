//
// Created by dylan on 6/14/2025.
//

#include "add.h"


//Adds a file to the staging area.
void add (string file) {
    if (!filesystem::exists(".mygit/")) {
        cout << "Must initialize a mygit repository first using mygit init." << endl;
        exit(1);
    }

    //Read the file and return it as a string.
    string fileToAdd = readFile(file);

    int bytes = fileToAdd.size();

    string blobString = "blob " + to_string(bytes) + '\0' + fileToAdd;

    string hashedBlobString = sha256(blobString);

    string hashFolderName = hashedBlobString.substr(0,2);
    string hashFileName = hashedBlobString.substr(2, hashedBlobString.length());

    if (!filesystem::exists(".mygit/objects/" + hashFolderName + "/" + hashFileName)) {
        filesystem::create_directory(".mygit/objects/" + hashFolderName);
        ofstream hashFile (".mygit/objects/" + hashFolderName + "/" + hashFileName);
        hashFile << blobString;
    }

    //Add file to the staging area for commit.
    ofstream indexFile(".mygit/index", ios::app);
    if (indexFile.is_open()) {
        string indexContents = hashedBlobString + " " + file + "\n";
        indexFile << indexContents;
        indexFile.close();
        cout << "added " << file << " to the staging area." << endl;

    } else {
        cout << "Error opening index file." << endl;
    }
}