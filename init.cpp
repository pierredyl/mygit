//
// Created by dylan on 6/14/2025.
//



#include "init.h"

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