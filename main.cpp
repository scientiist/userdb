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
//
#pragma endregion



///~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include "json.h"

using json = nlohmann::json; // This header is 20k lines LOL

#pragma region Utility Methods
// Utility Methods
void string_toupper(std::string& input)
{
    std::transform(input.begin(), input.end(), input.begin(), ::toupper);
}
void string_tolower(std::string& input)
{
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
}
#pragma endregion

#pragma region Lockfile

// Lockfile => Used to force UserDB to run effectively sequentially
#define LOCKFILE_NAME "userdb.lock"
bool locked()
{
    std::ifstream locker(LOCKFILE_NAME);

    std::string readout;

    locker >> readout;

    if (readout=="true")
    {
        locker.close();
        return true;
    }
    locker.close();
    return false;
}
bool lock()
{
    std::ofstream locker(LOCKFILE_NAME);
    if (locker.is_open())
    {
        locker << "true";
        locker.close();
        return true;
    }
    return false;

}
bool unlock()
{
    std::ofstream locker(LOCKFILE_NAME);
    if (locker.is_open())
    {
        locker << "false";
        locker.close();
        return true;
    }
    return false;
}
#pragma endregion

namespace UserDB
{

    struct Command
    {
        std::string name;
        std::vector<std::string> aliases;
        std::string description;
        std::string argsFormat;
        int(*Callback)(std::vector<std::string>);
    };

    const Command Commands[] = {
        {"help", {"-h", "h", "-help", "--h"}, "Help Command", "<command>"},
        {""},
    };
    bool Matches(std::string needle, std::vector<std::string> haystack)
    {
        for (int i = 0; i< haystack.size(); i++)
        {
            if (needle == haystack[i])
            {
                return true;
            }
        }
        return false;
    }

    const char* USERDB_FILE_NAME = "userdb.json";

    bool FindKey(const json& obj, const std::string key)
    {
        //std::cout << obj.type_name() << std::endl;

        for (auto& el: obj.items())
        {

            if (el.key() == key)
            {

                return true;
            } else {
                if (obj.type_name()!="string"&&obj.type_name()!="array")
                {
                    auto res = FindKey(obj[el.key()], key);
                    if (!res)
                    {
                        continue;
                    }
                    else
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    bool FindValue(const json& obj, const std::string key)
    {
        //std::cout << obj.type_name() << std::endl;

        for (auto& el: obj.items())
        {
            if (el.value() == key)
            {

                return true;
            } else {
                if (obj.type_name()!="string"&&obj.type_name()!="array")
                {
                    auto res = FindValue(obj[el.key()], key);
                    if (!res)
                    {
                        continue;
                    }
                    else
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    json GetJson(std::string filename)
    {
        std::fstream f(filename, std::ios_base::in | std::ios::binary);
        return json::parse(f);
    }
    void SetJson(std::string filename, json data)
    {
        std::ofstream of(filename); // TODO: Verify Params here
        of << data; // re-write json object to file;
        of.close();
    }

    int Get(std::string userid, std::string key)
    {
        json obj = GetJson(USERDB_FILE_NAME);
        json b = obj[userid][key];
        if (b.is_array())
        {
            for (auto& element: b)
                std::cout << element << std::endl;
            return 0;
        }
        std::cout << obj[userid][key] << std::endl;
        return 0;
    }
    // Set value in user entry
    int Set(std::string userid, std::string key, std::string json_literal)
    {
        json obj = GetJson(USERDB_FILE_NAME);
        obj[userid][key] = json_literal;
        SetJson(USERDB_FILE_NAME, obj);
        return 0;
    }
    // Dump all contents of user entry
    int Dump(std::string userid)
    {
        json obj = GetJson(USERDB_FILE_NAME);
        json b = obj[userid];
        std::cout << b << std::endl;
        if (b.is_array())
        {
            for (auto& element: b)
                std::cout << element << std::endl;
            return 0;
        }
        return 0;
    }
     std::string FindValueInAnyUser(std::string value)
     {
        json obj = GetJson(USERDB_FILE_NAME);
        for (auto& useracc: obj)
        {
            if (FindValue(useracc, value))
            {
                return useracc["username"];
            }
        }
        return "null";
    }
    int FindKeyInAnyUser(std::string key)
    {
        json obj = GetJson(USERDB_FILE_NAME);
        for (auto& useracc: obj)
        {
            if (FindKey(useracc, key))
            {
                std::cout << "Found in " << useracc["username"] << std::endl;
                return 0;
            }
        }
    }
    std::string CheckInvite(std::string inviteCode)
    {
        json obj = GetJson(USERDB_FILE_NAME);

        for (auto& useracc: obj)
        {
            json invites = useracc["invites"];
            if (invites.is_array())
            {
                for (auto& element: invites)
                {
                    if (element.is_string() && element == inviteCode)
                        return useracc["username"];
                }
            }
        }
        return "null";
    }
    bool RemoveUser(std::string userid)
    {
        json obj = GetJson(USERDB_FILE_NAME);

        if (obj.contains(userid))
        {
            obj.erase(userid);
            return true;
        }
        return false;
    }

    void HelpInfo(std::string commandID)
    {
        for (auto& cmd: Commands)
        {
            if (cmd.name == commandID || Matches(commandID, cmd.aliases))
            {
                std::cout << "Cmd: " << commandID << std::endl;
                std::cout << "Desc: " << cmd.description << std::endl;
                std::cout << "Usage: userdb " << commandID << " " << cmd.argsFormat << std::endl;
                return;
            }
        }

        std::cout << "No such command " << commandID << " found!" << std::endl;
    }

    void HelpInfo()
    {
        std::cout << "UserDB by J. O'Leary" << std::endl;
        std::cout << "Usage: userdb <command> [arguments] [-flags]" << std::endl;
        std::cout << "---------------------------------------------" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "help|h|--h <command>" << std::endl;
        std::cout << "get|get_user_key <userid> <key>" << std::endl;
        std::cout << "set|set_user_key <userid> <key> <value>" << std::endl;
        std::cout << "dump <userid>" << std::endl;
        std::cout << "search_k <key>" << std::endl;
        std::cout << "search_v <value>" << std::endl;
        std::cout << "add_user <userid> <json?>" << std::endl;
        std::cout << "remove_user <userid>" << std::endl;
        std::cout << "is_username_taken <username>" << std::endl;
        std::cout << "checkinv <invitecode>" << std::endl;
        std::cout << "useinv <invitecode>" << std::endl;
    }

typedef std::vector<std::string> ArgsList;

#pragma region Command Functions
    int HelpCommand(ArgsList &args)
    {
        if (args.size() > 0)
        {
            HelpInfo(args[0]);
            return 0;
        }

        HelpInfo();
        return 0;
    }
    // Reads user key
    int GetCommand(ArgsList &args)
    {
        if (args.size() < 2)
        {
            std::cout << "Usage: userdb get <userid> <key>" << std::endl;
            return -1;
        }

        std::string userid = args[0];
        std::string key    = args[1];

        return Get(userid, key);
    }
    int SetCommand(ArgsList &args)
    {
        if (args.size() < 3)
        {
            std::cout << "Usage: userdb set <userid> <key> <value>" << std::endl;
            return -1;
        }
        std::string userid = args[0];
        std::string key    = args[1];
        std::string value  = args[2];

        return Set(userid, key, value);
    }
    int DumpCommand(ArgsList &args)
    {
        if (args.size() < 1)
        {
            std::cout << "Usage: userdb dump <userid>" << std::endl;
            return -1;
        }
        std::string userid = args[0];
        return Dump(userid);
    }
    int SearchAccountsKeyCommand(ArgsList &args)
    {
        if (args.size() < 1)
        {
            std::cout << "Usage: userdb search_k <idmatch>" << std::endl;
            return -1;
        }
        std::string key = args[0];
        return FindKeyInAnyUser(key);

    }
    int SearchAccountsValueCommand(ArgsList &args)
    {
        if (args.size() < 1)
        {
            std::cout << "Usage: userdb search_v <idmatch>" << std::endl;
            return -1;
        }
        std::string key = args[0];
        std::string result = FindValueInAnyUser(key);
        if (result != "null")
        {
            std::cout << result << std::endl;
            return 1;
        }

        return -1;
    }
    int RemoveUserCommand(ArgsList &args)
    {
        if (args.size() < 1)
        {
            std::cout << "Usage: userdb remove <userid>" << std::endl;
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
    int RemoveUserKeyCommand(ArgsList &args)
    {

    }
    int AddUserCommand(ArgsList &args)
    {

    }
    int IsUsernameTakenCommand(ArgsList &args)
    {

    }
    int CheckInviteCommand(ArgsList &args)
    {
        std::cout << CheckInvite(args[0]) << std::endl;
    }
    int UseInviteCommand(ArgsList &args)
    {
        std::string userid = CheckInvite(args[0]);

    }


#pragma endregion
    int ParseArgs(ArgsList args)
    {
        std::string command = args[0];
        args.erase(args.begin());

        if (Matches(command, {"help", "-help", "--help", "h", "-h"}))
        { return HelpCommand(args); }
        if (Matches(command, {"get", "get_user_key"}))
        { return GetCommand(args);  }
        if (Matches(command, {"set", "set_user_key"}))
        { return SetCommand(args);  }
        if (command == "remove_user")
        { return RemoveUserCommand(args); }
        if (command == "remove_user_key")
        { return RemoveUserKeyCommand(args); }
        if (command == "add_user")
        { return AddUserCommand(args); }
        if (command == "is_username_taken")
        { return IsUsernameTakenCommand(args); }
        // Enumerate all attributes of the user
        if (command == "dump")
        {return DumpCommand(args); }
        if (command == "search_k")
        { return SearchAccountsKeyCommand(args);}
        if (command == "search_v")
        { return SearchAccountsValueCommand(args);}
        if (command == "checkinv")
        { return CheckInviteCommand(args);}
        if (command == "useinvite")
        { return UseInviteCommand(args);}
        return -1;
    }
    int ParseArgs(int argc, char** argv) {
        if (argc < 2)
            return -1;
        return ParseArgs(std::vector<std::string>(argv+1, argv+argc));
    }

}

void erase( std::string &str, char charToRemove )
{
    str.erase( remove(str.begin(), str.end(), charToRemove), str.end() );
}


#define EXPLODE_IF_LOCKED true

int main(int argc, char **argv)
{

#if EXPLODE_IF_LOCKED
    if (locked())
    {
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
    //lock();

    argv[argc] = "help";
    argc++;

    int result = UserDB::ParseArgs(argc, argv);

    if (result == -1)
        UserDB::HelpInfo();

    //unlock();
    return result;
}
