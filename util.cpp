//
// Created by dylan on 6/14/2025.
//


#include "util.h"


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
    file.close();

    return buffer.str();
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



//Compresses a vector of chars using the DEFLATE algorithm from zlib.
vector<unsigned char> compressUsingDeflate(vector<unsigned char> original) {
    vector<unsigned char> result;

    z_stream defstream{};
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;

    unsigned char out[CHUNK_SIZE];
    size_t have;

    int ret = deflateInit(&defstream, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) {
        cout << "deflateInit failed" << endl;
        return {};
    }

    //Size of bytes in vector
    defstream.avail_in =  original.size();

    //Pointer to first location in memory of vector
    defstream.next_in = original.data();


    do {
        defstream.next_out = out;
        defstream.avail_out = CHUNK_SIZE;

        ret = deflate(&defstream, Z_FINISH);

        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR) {
            cout << "deflate failed with code: " << ret << endl;
            deflateEnd(&defstream);
            return {};
        }

        have = CHUNK_SIZE - defstream.avail_out;
        result.insert(result.end(), out, out + have);

    } while (defstream.avail_out == 0);

    // clean up and return
    (void)deflateEnd(&defstream);

    return result;
}




//Decompresses a file using zlib and its inflate algorithm
vector<unsigned char> decompressUsingInflate(vector<unsigned char> original) {
    vector<unsigned char> result;

    int ret;
    z_stream defstream{};
    size_t have;

    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;

    unsigned char out[CHUNK_SIZE];


    ret = inflateInit(&defstream);
    if (ret != Z_OK) {
        cout << "inflateInit failed" << endl;
        return {};
    }

    defstream.avail_in = original.size();
    defstream.next_in = original.data();

    do {
        defstream.next_out = out;
        defstream.avail_out = CHUNK_SIZE;

        ret = inflate(&defstream, Z_NO_FLUSH);

        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR) {
            cout << "inflate failed: " << ret << endl;
            inflateEnd(&defstream);
            return {};
        }


        have = CHUNK_SIZE - defstream.avail_out;
        result.insert(result.end(), out, out + have);

    } while (defstream.avail_out == 0);

    (void)inflateEnd(&defstream);

    return result;
}




//Function to handle writing binary data to a file.
void writeBinaryToFile(const string& filename, vector<unsigned char>& data) {
    ofstream file(filename, ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    } else {
        cout << "Failed creating file " << filename << endl;
    }

    file.close();
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



//Parses the head file for the branch information. Returns this branch information.
string parseHeadForBranch(ifstream& headFile) {
    string line;

    //only check for main right now. Can support other branches later. Simple parsing.
    while (getline(headFile, line)) {
        if (line.find("ref:") != string::npos) {
            string branchLocation = line.substr(line.find_first_of(":") + 2, line.length());
            headFile.close();
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

