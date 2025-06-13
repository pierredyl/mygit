#include <iostream>
#include <filesystem>
#include <fstream>
#include <openssl/sha.h>

using namespace std;


//Reads a file into a string. Returns the string representation of the entire file.
string readFile(const string& filePath) {
    ifstream file = ifstream(filePath, ios::binary);

    if (!file) {
        cout << "Error opening file at " << filePath << endl;
        exit(1);
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

//Initializes a local mygit repository.
void init () {
    string repoPath = ".mygit/";

    if (!filesystem::exists(repoPath)) {
        filesystem::create_directory(repoPath);

        string objectsPath = repoPath + "objects/";
        string refsPath = repoPath + "refs/";

        filesystem::create_directory(objectsPath);
        filesystem::create_directory(refsPath);
        ofstream headFile (repoPath + "HEAD");
        ofstream indexFile (repoPath + "index");
    } else {
        cout << "mygit repository already created" << endl;
    }
}

//Hashes an input string using the sha256 technique.
string sha256(string s) {
    unsigned char hash[SHA256_DIGEST_LENGTH];

    //Parameters to OpenSSL SHA256 are pointers to character strings.
    SHA256(reinterpret_cast<const unsigned char*>(s.c_str()), s.size(), hash);

    //Create a stringstream
    stringstream stream;

    //Iterate over the entire hash table
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        //Convert each hash from hex to its string interpretation. Store in the stringstream.
        stream << hex << setw(2) << setfill('0') << int(hash[i]);
    }

    return stream.str();


}

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
    string hashFileName = hashedBlobString.substr(2, hashedBlobString.length() - 1);

    if (!filesystem::exists(".mygit/objects/" + hashFolderName + "/" + hashFileName)) {
        filesystem::create_directory(".mygit/objects/" + hashFolderName);
        ofstream hashFile (".mygit/objects/" + hashFolderName + "/" + hashFileName);
        hashFile << blobString;
    }

}




int main(int argc, char* argv[]) {
    cout << "Running mygit" << endl;

    if (argc < 2) {
        cout << "Usage: ./mygit [argument]" << endl;
        return 1;
    }

    string command = argv[1];

    if (command == "init") {
        init();
    } else if (command == "add") {
        string fileToAdd = argv[2];
        add(fileToAdd);
    }
}
