//
// Created by dylan on 6/14/2025.
//

#include "commit.h"

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