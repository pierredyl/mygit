//
// Created by dylan on 6/15/2025.
//

#include "config.h"

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