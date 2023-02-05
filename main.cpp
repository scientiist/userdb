/// UserDB v0.2 - A scuffed JSON database written in C++
/// @auth Joshua "brogrammer" O'Leary
/// @repo https://github.com/scientiist/userdb
/// @url http://conarium.software

#pragma region License
/* License (CSOFT-1)
 * Copyright 2023 Conarium Software LLC
 * Redistribution and use in source and binary forms,
 * with or without modification,
 * are permitted without condition.
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPERS "AS-IS"
 * NO WARRANTY IS EXPRESSED WHATSOEVER, AND THE DEVELOPER IS NOT
 * LIABLE FOR ANY DAMAGES, ACCIDENTAL OR OTHERWISE.
 * IN OTHER WORDS: USE AT YOUR OWN PERIL
 */
#pragma endregion
#pragma region Documentation
/*
 *
 */
#pragma endregion
#pragma region Compiling
/* Compilation of this software should be very straightforward:
 * Simply run the main.cpp file through gcc (or your compiler of choice)
 * For example, on linux:
 * g++ main.cpp -o userdb
 */
#pragma endregion
#pragma region Todo List
// TODO LIST
// Refactor command "metadata" to not be spread across the source. I.e. Create Command Structs
// Thoroughly Unit test each command
// Make >1 args for Set command apply as an array;
//
#pragma endregion
///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <iostream>
#include <fstream>
#include <thread>
#include <algorithm>
#include "json.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-use-anyofallof"
#pragma region Utility Methods

// Utility Methods
void string_tolower(std::string &input) {
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
}

#pragma endregion

#pragma region Lockfile

// Lockfile => Used to force UserDB to run effectively sequentially
#define LOCKFILE_NAME "userdb.lock"

bool locked() {
    std::ifstream locker(LOCKFILE_NAME);

    std::string readout;

    locker >> readout;

    if (readout == "true") {
        locker.close();
        return true;
    }
    locker.close();
    return false;
}

bool lock() {
    std::ofstream locker(LOCKFILE_NAME);
    if (locker.is_open()) {

        locker << "true";
        locker.close();
        return true;
    }
    return false;

}

bool unlock() {
    std::ofstream locker(LOCKFILE_NAME);
    if (locker.is_open()) {
        locker << "false";
        locker.close();
        return true;
    }
    return false;
}

#pragma endregion

namespace UserDB {
    using json = nlohmann::json; // This header is 20k lines LOL

    struct Command;

    Command& getCommand(std::string&);

    bool hasCommand(std::string& command);

    std::vector<Command> &getCommands();

    bool Matches(const std::string &needle, const std::vector<std::string> &haystack) {
        for (auto &potential: haystack)
            if (needle == potential)
                return true;
        return false;
    }

    const char *USERDB_FILE_NAME = "userdb.json";

    bool FindKey(const json &obj, const std::string &key) {
        for (auto &el: obj.items()) {
            if (el.key() == key) {
                return true;
            } else {
                std::string type_name = std::string(obj.type_name());
                if (type_name != "string" && type_name != "array") {
                    auto res = FindKey(obj[el.key()], key);
                    if (!res)
                        continue;
                    else
                        return true;
                }
            }
        }
        return false;
    }

    bool FindValue(const json &obj, const std::string &key) {
        for (auto &el: obj.items()) {
            if (el.value() == key)
                return true;
            else {
                if (obj.type_name() != "string" && obj.type_name() != "array") {
                    auto res = FindValue(obj[el.key()], key);
                    if (!res)
                        continue;
                    else
                        return true;
                }
            }
        }
        return false;
    }

    json GetJson(const std::string &filename) {
        std::fstream f(filename, std::ios_base::in | std::ios::binary);
        return json::parse(f);
    }

    void SetJson(const std::string &filename, json &data) {
        std::ofstream of(filename); // TODO: Verify Params here
        of << data.dump(4); // re-write json object to file;
        of.close();
    }

    std::string Get(const std::string &userid, const std::string &key) {
        json obj = GetJson(USERDB_FILE_NAME);
        json b = obj[userid][key];
        std::string ret;
        if (b.is_array())
            for (auto &element: b)
                ret += element.dump() + " ";
        else
            ret = b.dump();
        return ret;
    }

    // Set value in user entry
    int Set(const std::string &userid, const std::string &key, const std::string &json_literal) {
        json obj = GetJson(USERDB_FILE_NAME);
        obj[userid][key] = json_literal;
        SetJson(USERDB_FILE_NAME, obj);
        return 0;
    }


    // Dump all contents of user entry
    int Dump(std::string& userid) {
        json obj = GetJson(USERDB_FILE_NAME);

        if (!obj.contains(userid))
        {
            std::cout << "User " << userid << " not found!" << std::endl;
            return 1;
        }

        json userindex = obj[userid];

        std::cout << userindex << std::endl;
        if (userindex.is_array()) {
            for (auto &element: userindex) {
                std::string rawText = element.dump();
                std::cout << rawText << std::endl;
            }
        }
        return 0;
    }

    std::string FindValueInAnyUser(std::string& value) {
        json obj = GetJson(USERDB_FILE_NAME);
        for (auto &useracc: obj) {
            if (FindValue(useracc, value)) {
                return useracc["username"];
            }
        }
        return "null";
    }

    int FindKeyInAnyUser(std::string& key) {
        json obj = GetJson(USERDB_FILE_NAME);
        for (auto &useracc: obj) {
            if (FindKey(useracc, key)) {
                std::cout << "Found in " << useracc["username"] << std::endl;
                return 0;
            }
        }
    }

    bool IsUserIDTaken(std::string& userid)
    {

        json obj = GetJson(USERDB_FILE_NAME);

        if (obj.contains(userid))
            return true;
        return false;
    }

    std::string CheckInvite(std::string& inviteCode) {
        json obj = GetJson(USERDB_FILE_NAME);

        for (auto &useracc: obj) {
            json invites = useracc["invites"];
            if (invites.is_array()) {
                for (auto &element: invites) {
                    if (element.is_string() && element == inviteCode)
                        return useracc["username"];
                }
            }
        }
        return "null";
    }

    bool RemoveUser(std::string& userid) {
        json obj = GetJson(USERDB_FILE_NAME);

        if (obj.contains(userid)) {
            obj.erase(userid);
            return true;
        }
        return false;
    }


    struct Command {
        std::string name;
        std::vector<std::string> aliases;
        std::string description;
        std::string argsFormat;

        int (*Callback)(std::vector<std::string> &);

        std::string getAliasesString()
        {
            std::string ret;
            ret.append(name);
            if (this->aliases.empty()) {return ret;}
            for (auto& alias : this->aliases)
            {
                ret.append("|");
                ret.append(alias);

            }
        }
    };


    void HelpInfo(std::string commandID) {
        if (hasCommand(commandID)) {
            Command cmd = getCommand(commandID);
            std::cout << "Cmd: " << commandID << std::endl;
            std::cout << "Desc: " << cmd.description << std::endl;
            std::cout << "Usage: userdb " << commandID << " " << cmd.argsFormat << std::endl;
            return;
        }


        std::cout << "No such command " << commandID << " found!" << std::endl;
    }

    void HelpInfo() {
        std::cout << "UserDB by J. O'Leary" << std::endl;
        std::cout << "Usage: userdb <command> [arguments] [-flags]" << std::endl;
        std::cout << "---------------------------------------------" << std::endl;
        std::cout << "Commands:" << std::endl;
        for (auto& command: getCommands()) {
            std::cout << command.getAliasesString() << " " << command.argsFormat << " - " << command.description << std::endl;
        }
    }

    typedef std::vector<std::string> ArgsList;

#pragma region Command Functions

    int HelpCommand(ArgsList &args) {
        if (args.empty()){
            HelpInfo();
        } else if (args.size() > 1) {
            HelpInfo(args[0]);
        }
        return 0;
    }

    // Reads user key
    int GetCommand(ArgsList &args) {
        if (args.empty()){
            return -1;
        }
        if (args.size() < 2) {
            return -1;
        }

        std::string userid = args[0];
        std::string key = args[1];

        std::string result = Get(userid, key);
        std::cout << result << std::endl;
        return 0;
    }

    int SetCommand(ArgsList &args) {
        if (args.empty()){
            return -1;
        }
        if (args.size() < 3) {
            return -1;
        }
        std::string userid = args[0];
        std::string key = args[1];
        std::string value = args[2];

        return Set(userid, key, value);
    }

    int DumpCommand(ArgsList &args) {
        if (args.empty()){
            return -1;
        }
        std::string userid = args[0];
        return Dump(userid);
    }

    int SearchAccountsKeyCommand(ArgsList &args) {
        if (args.empty()){
            return -1;
        }
        std::string key = args[0];
        return FindKeyInAnyUser(key);

    }

    int SearchAccountsValueCommand(ArgsList &args) {
        if (args.empty()) {
            return -1;
        }
        std::string key = args[0];
        std::string result = FindValueInAnyUser(key);
        if (result != "null") {
            std::cout << result << std::endl;
            return 1;
        }

        return -1;
    }

    int RemoveUserCommand(ArgsList &args) {
        if (args.empty()) {
            return -1;
        }
        bool success = RemoveUser(args[0]);
        if (success) {
            std::cout << "true" << std::endl;
            return 0;
        } else {
            std::cout << "false" << std::endl;
            return 0;
        }
    }

    int RemoveUserKeyCommand(ArgsList &args) {

        if (args.size() < 2) {
            return -1;
        }

        return -0;
    }

    int AddUserCommand(ArgsList &args) {
        if (args.size() < 2) {
            return -1;
        }
        return -0;
    }

    int IsUsernameTakenCommand(ArgsList &args) {
        if (args.empty()) {
            return -1;
        }

        std::string result = IsUserIDTaken(args[0]) ? "true" : "false";

        std::cout << result << std::endl;

        return 0;
    }

    int CheckInviteCommand(ArgsList &args) {
        std::cout << CheckInvite(args[0]) << std::endl;
        return 0;
    }

    int UseInviteCommand(ArgsList &args) {
        std::string userid = CheckInvite(args[0]);
        return 0;
    }

    int CountInstancesCommand(ArgsList &args) {
        if (args.empty()) {
            std::cout << "BAH" << std::endl;
            return 0;
        }
        std::string keyword = args[0];
    }

    int AddCommand(ArgsList &args)
    {
        if (args.empty())
        {

        }
    }

    std::vector<Command> Commands = {
            {"help",              {"-h"}, "Help Command", "<command>",      HelpCommand},
            {"get",               {"get_user_key"},            "Retrieves a user value.",  "<userid> <key>", GetCommand},
            {"set",               {"set_user_key"},            "Stores a user value.",     "<userid> <key>", SetCommand},
            {"add", {}, "Adds an entry to a user-array specifically.", "<userid> <key> <insert>", AddCommand},
            {"remove_user",       {}, "Removes a user entry from the database.", "<userid>", RemoveUserCommand},
            {"remove_user_key",   {}, "Removes a key from a user entry.", "<userid> <key>", RemoveUserKeyCommand},
            {"add_user",          {}, "Add a user entry to the database.",  "<userid>", AddUserCommand},
            {"is_username_taken", {}, "Checks if username is already in use.","<userid>", IsUsernameTakenCommand},
            {"dump",              {}, "Dumps a user entry as raw JSON.", "<userid>",      DumpCommand},
            {"search_k",          {}, "Recursive searches the entire database for a specified key.", "<key>",               SearchAccountsKeyCommand},
            {"search_v",          {}, "Recursive searches the entire database for a specified value.", "<value>",               SearchAccountsValueCommand},
            {"checkinv",          {}, "Checks a user invite code for validity: Returns the owner of the code.", "<invitecode>",               CheckInviteCommand},
            {"useinvite",         {}, "Authenticates and consumes a user invite code. Returns the owner of the code.", "<invitecode>",               UseInviteCommand},
            {"count", {}, "counts instances of a specified keyword throughout the database", "<keyword>", CountInstancesCommand},
            };

    // Kinda Gross Workaround bc im too lazy for good design rn
    bool hasCommand(std::string& command) {
        for (auto &cmd: Commands)
            if (cmd.name == command || Matches(command, cmd.aliases))
                return true;
        return false;
    }


    class InvalidCommandException {
    public:
        std::string InvalidCommand;
        InvalidCommandException(std::string command)
        {
            this->InvalidCommand = command;
        }
    };

    Command& getCommand(std::string& key) {
        for (auto &cmd: Commands)
            if (cmd.name == key || Matches(key, cmd.aliases))
                return cmd;
        throw InvalidCommandException(key);
    }
    std::vector<Command> &getCommands() { return Commands; }



#pragma endregion

    int ParseArgs(ArgsList args) {
        std::string command = args[0];
        args.erase(args.begin());
        for (std::string &arg: args) {
            string_tolower(arg);
        }

        for (Command &cmd: Commands) {
            if (cmd.name == command || Matches(command, cmd.aliases)) {
                int result = cmd.Callback(args);
                if (result < 0) {
                    std::cout << "Usage: ./userdb " << cmd.name << " " << cmd.argsFormat << std::endl;
                    return result;
                }
                return result;
            }
        }
        return -1;
    }

    int ParseArgs(int argc, char **argv) {
        if (argc < 2)
            return -1;
        return ParseArgs(std::vector<std::string>(argv + 1, argv + argc));
    }

}

void erase(std::string &str, char charToRemove) {
    str.erase(remove(str.begin(), str.end(), charToRemove), str.end());
}

#define EXPLODE_IF_LOCKED false

int main(int argc, char **argv) {

#if EXPLODE_IF_LOCKED
    if (locked()) {
        std::cout << "null\n(UserDB is currently locked)" << std::endl;
        return -1;
    }
#else
    while (locked())
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }
#endif
    lock();

    int result = UserDB::ParseArgs(argc, argv);

    if (result == -1)
        UserDB::HelpInfo();

    unlock();
    return result;
}

#pragma clang diagnostic pop