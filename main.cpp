#include <iostream>
#include <filesystem>
#include <fstream>

#include "init.h"
#include "commit.h"
#include "add.h"
#include "config.h"

using namespace std;


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
    } else if (command == "log") {
        commitLog();
    }
}
