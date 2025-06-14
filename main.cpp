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
constexpr int CHUNK_SIZE = 16384;



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




//Creates the paths and files necessary for a mygit repository.
void createRootPaths() {
    string repoPath = ".mygit/";

    if (!filesystem::exists(repoPath)) {
        filesystem::create_directory(repoPath);

        string objectsPath = repoPath + "objects/";
        string refsPath = repoPath + "refs/";
        string headsRefsPath = refsPath + "heads/";

        filesystem::create_directory(objectsPath);
        filesystem::create_directory(refsPath);
        filesystem::create_directory(headsRefsPath);

        ofstream headFile (repoPath + "HEAD");
        ofstream indexFile (repoPath + "index");
        ofstream refsFile (headsRefsPath + "main");

        //Write current state to HEAD file
        indexFile.close();
        refsFile.close();

        if (headFile.is_open()) {
            string refsLink = "ref: refs/heads/main";
            headFile << refsLink;
            headFile.close();
        } else {
            cout << "Failed writing to headFile" << endl;
            headFile.close();
        }


        cout << "created a mygit repository on the main branch" << endl;
    } else {
        cout << "mygit repository already created" << endl;
    }
}




//Initializes a local mygit repository.
void init () {
    createRootPaths();
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



pair<string, string> parseConfigFileForUser() {
    ifstream configFile(".mygit/config");

    if (!configFile.is_open()) {
        cout << "Config file failed to open" << endl;
        exit(1);
    }

    string line;
    string name;
    string email;


    while (getline(configFile, line)) {
        if (line.find("name =") != string::npos) {
            name = line.substr(line.find_first_of("=") + 2, line.length());
        } else if (line.find("email =") != string::npos) {
            email = line.substr(line.find_first_of("=") + 2, line.length());
            break;
        }
    }

    configFile.close();

    return make_pair(name, email);
}




string parseHeadForBranch(ifstream& headFile) {
    string line;

    //only check for main right now. Can support other branches later. Simple parsing.
    while (getline(headFile, line)) {
        if (line.find("ref:") != string::npos) {
            string branchLocation = line.substr(line.find_first_of(":") + 2, line.length());
            return branchLocation;
        }
    }
    exit(1);
}




//Collects all index entries from the index file and stores them in a vector of <IndexEntry>
vector<IndexEntry> collectAllIndexEntries() {
    vector<IndexEntry> entries;
    ifstream indexFile(".mygit/index");
    string line;

    if (indexFile.is_open()) {
        while (getline(indexFile, line)) {
            istringstream iss(line);
            IndexEntry entry;
            iss >> entry.hashString >> entry.path;
            entries.push_back(entry);
        }
    } else {
        cout << "Error opening index file" << endl;
    }

    indexFile.close();
    return entries;
}


//Builds the commit tree from all of the entries in the index file and returns the hash of the tree.
string buildCommitTree(vector<IndexEntry>& indexEntries) {
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

    return hashedTree;
}




//Builds the commit object given a commit tree is made and a message is provided.
void buildCommitObject(string hashedTree, string& message) {

    string mainBranchFile = ".mygit/refs/heads/main";
    string commitData;

    //Build commit object. <user, email>
    pair<string, string> userInfo;

    //Get user info from config file
    userInfo = parseConfigFileForUser();

    //Before comitting current object, check if there is a previous commit. If there isn't, commit object has
    //no parent hash.
    if (!filesystem::is_empty(mainBranchFile)) {

        //Read the previous commit hash
        ifstream file(mainBranchFile);

        if (file.is_open()) {
            string previousCommitHash;
            //Branch file will only have one hash, which is the previous commit objects hash.
            getline(file, previousCommitHash);

            commitData = "tree " + hashedTree + "\n" + "parent " + previousCommitHash + "\n" +"author "
            + userInfo.first + " <" + userInfo.second + ">" + "\n" + "committer "
            + userInfo.first +" <" + userInfo.second + ">" + "\n" + message;
        } else {
            cout << "Unable to open main branch file" << endl;
        }
    } else {
        commitData = "tree " + hashedTree + "\n" + "author " + userInfo.first + " <" + userInfo.second + ">" + "\n" +
            "committer " + userInfo.first + " <" + userInfo.second + ">" + "\n" + message;
    }

    cout << "\ncomitting data: \n" << commitData;

    int commitSize = commitData.size();
    string commitObject = "commit " + to_string(commitSize) + '\0' + commitData;
    string commitObjectHash = sha256(commitObject);
    cout << "Creating a new commit object with hash: " << commitObjectHash << endl;

    //create directory for commitFile.
    filesystem::create_directory(".mygit/objects/" + commitObjectHash.substr(0,2));

    string commitFileDir = ".mygit/objects/" + commitObjectHash.substr(0, 2) + "/" +
        commitObjectHash.substr(2, commitObjectHash.length());

    //Convert the commitObject into a vector of characters for compression.
    vector<unsigned char> commitObjectVec(commitObject.begin(), commitObject.end());

    vector<unsigned char> compressedObjectFile = compressUsingDeflate(commitObjectVec);

    //Write the compressed commit object to its file
    writeBinaryToFile(commitFileDir, compressedObjectFile);

    //Update HEAD
    ifstream headFile(".mygit/HEAD");
    string branchLocation = parseHeadForBranch(headFile);

    //Write commit hash to branch file inside heads.
    ofstream branchFileLocation(".mygit/" + branchLocation);

    if (branchFileLocation.is_open()) {
        branchFileLocation << commitObjectHash;
    } else {
        cout << "Failed to open branch heads file" << endl;
    }
}




//Handles commits. Retrieves all data from the index file, places it into a vector of structs,
//converts hex string hashes to their binary interpretation, constructs the tree using that
//binary interpretation, hashes that file for its filename, and writes its contents to that file.
void commit(string& message) {

    //Store all hashes and their corresponding files in a vector of entries.
    vector<IndexEntry> indexEntries = collectAllIndexEntries();

    //Convert hash strings to their binary representation and store it.
    for (int i = 0; i < indexEntries.size(); i++) {
        indexEntries[i].hashBinary = hashStringToBinary(indexEntries[i].hashString);
    }

    //Call buildCommitTree to build the tree, and get the hash of that tree.
    string hashedTree = buildCommitTree(indexEntries);

    //Call buildCommitObject to build the commit object using the commit message and tree hash.
    buildCommitObject(hashedTree, message);
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




void config() {
    if (!filesystem::exists(".mygit/config")) {
        ofstream configFile(".mygit/config");

        string name;
        string email;

        cout << "enter name: ";
        getline(cin, name);
        cout << "enter email: ";
        getline(cin, email);

        configFile << "[user]\n" << "name = " << name << "\nemail = " << email << endl;

        cout << "created config file for: " << name << endl;
    } else {
        cout << "Config file already exists." << endl;
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
        if (argc < 3) {
            cout << "Usage: ./mygit commit -m [message]" << endl;
            return 1;
        }

        string commitMessage;
        for (int i = 3; i < argc; i++) {
            commitMessage += string(argv[i]) + " ";
        }

        commit(commitMessage);

    } else if (command == "config") {
        config();
    }
}
