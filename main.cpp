#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <openssl/sha.h>
#include <zlib.h>

using namespace std;

struct IndexEntry {
    string hashString;
    string path;
    vector<unsigned char> hashBinary;
};

const string NORMAL_FILE_MODE = "100644";
const int CHUNK_SIZE = 16384;



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




//Converts a hex character to its numerical representation.
unsigned char hexCharToNum(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
        return 10 + (ch - 'a');
    } else if (ch >= 'A' && ch <= 'F') {
        return 10 + (ch - 'A');
    } else {
        cout << "Invalid character in string" << endl;
        exit(1);
    }
}






vector<unsigned char> hashStringToBinary(string s) {
    //32 byte hash string (for SHA256)
    vector<unsigned char> hash;
    hash.reserve(32);

    for (int i = 0; i < s.length(); i += 2) {
        unsigned char highNibble = hexCharToNum(s.at(i));
        unsigned char lowNibble = hexCharToNum(s.at(i+1));
        unsigned char byte = highNibble << 4 | lowNibble;
        hash.push_back(byte);
    }

    return hash;
}



//Compresses a vector of chars using the DEFLATE algorithm from zlib.
vector<unsigned char> compressUsingDeflate(vector<unsigned char> original) {
    vector<unsigned char> result;

    z_stream defstream{};
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;

    deflateInit(&defstream, Z_DEFAULT_COMPRESSION);

    defstream.avail_in = original.size();

    //Points to the vectors first location in memory.
    defstream.next_in = original.data();

    unsigned char out[CHUNK_SIZE];

    do {
        defstream.next_out = out;
        defstream.avail_out = CHUNK_SIZE;

        deflate(&defstream, Z_FINISH);

        size_t have = CHUNK_SIZE - defstream.avail_out;
        result.insert(result.end(), out, out + have);
    } while (defstream.avail_out == 0);

    return result;
}



//Function to handle writing binary data to a file.
void writeBinaryToFile(const string& filename, vector<unsigned char>& data) {
    ofstream file(filename);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    } else {
        cout << "Failed creating file " << filename << endl;
    }
}



//Handles commits. Retrieves all data from the index file, places it into a vector of structs,
//converts hex string hashes to their binary interpretation, constructs the tree using that
//binary interpretation, hashes that file for its filename, and writes its contents to that file.
void commit() {

    //Store all hashes and their corresponding files in a vector of entries.
    vector<IndexEntry> indexEntries;

    ifstream indexFile(".mygit/index");
    string line;

    if (indexFile.is_open()) {
        while (getline(indexFile, line)) {
            istringstream iss(line);
            IndexEntry entry;
            iss >> entry.hashString >> entry.path;
            indexEntries.push_back(entry);
        }
    } else {
        cout << "Error opening index file" << endl;
    }

    indexFile.close();

    cout << "Reading from the staging area" << endl;
    for (int i = 0; i < indexEntries.size(); i++) {
        cout << indexEntries[i].hashString << " " << indexEntries[i].path << endl;
    }

    //Convert hash strings to their binary representation and store it.
    for (int i = 0; i < indexEntries.size(); i++) {
        indexEntries[i].hashBinary = hashStringToBinary(indexEntries[i].hashString);
    }

    vector<unsigned char> tree;
    //Construct the tree for all entries in the index.
    for (int i = 0; i < indexEntries.size(); i++) {
        tree.insert(tree.end(), NORMAL_FILE_MODE.begin(), NORMAL_FILE_MODE.end());
        tree.push_back(' ');
        tree.insert(tree.end(), indexEntries[i].path.begin(), indexEntries[i].path.end());
        tree.push_back('\0');
        tree.insert(tree.end(), indexEntries[i].hashBinary.begin(), indexEntries[i].hashBinary.end());
    }

    string treeContent;
    int treeLength = tree.size();

    treeContent = "tree " + to_string(treeLength) + '\0';
    treeContent.insert(treeContent.end(), tree.begin(), tree.end());

    //Convert treeContent string to vector<unsigned char>
    vector<unsigned char> treeContentVec(treeContent.begin(), treeContent.end());

    //Hashed tree to use for the file name in objects.
    string hashedTree = sha256(treeContent);

    //Split hash for proper file structure (as used in git)
    string hashedTreeDir = ".mygit/objects/" + hashedTree.substr(0, 2);

    //Create the directory for the hashed tree.
    if (!filesystem::exists(hashedTreeDir)) {
        filesystem::create_directory(hashedTreeDir);
    } else {
        cout << "Tree directory already exists." << endl;
    }

    string hashedTreeFilePath = hashedTreeDir + "/" + hashedTree.substr(3, hashedTree.size());

    if (!filesystem::exists(".mygit/objects/" + hashedTree)) {
        //Compress the tree
        vector<unsigned char> compressedTree = compressUsingDeflate(treeContentVec);

        //Write compressedTree to file
        writeBinaryToFile(hashedTreeFilePath, compressedTree);

        cout << "created tree object file at " << hashedTreeFilePath << endl;

    } else {
        cout << "tree already exists in objects." << endl;
    }
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
    } else if (command == "commit") {
        commit();
    }
}
