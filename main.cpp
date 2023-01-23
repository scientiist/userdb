//=> UserDB
// @auth Joshua "brogrammer" O'Leary
// Copyright 2023 Conarium Software LLC All Rights Reserved

//=> TODO LIST

//=>

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include "json.h"

using json = nlohmann::json; // This header is 20k lines LOL

#pragma region Utility Methods
// Utility Methods
std::string& string_toupper(std::string& input)
{
    std::transform(input.begin(), input.end(), input.begin(), ::toupper);
}
std::string& string_tolower(std::string& input)
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
    const char*  defaultErrorMessage = "";
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

    // Get Value in user entry
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
    int FindValueInAnyUser(std::string value)
    {
        json obj = GetJson(USERDB_FILE_NAME);
        for (auto& useracc: obj)
        {
            if (FindValue(useracc, value))
            {
                std::cout << "Found in " << useracc["username"] << std::endl;
                return 0;
            }
        }
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
    int RemoveUser(std::string userid)
    {

    }

    void HelpInfo(std::string command)
    {

    }

    void HelpInfo()
    {
        std::cout << "UserDB by J. O'Leary" << std::endl;
        std::cout << "Usage: userdb <command> [arguments] [-flags]" << std::endl;

        std::cout << "Commands:" << std::endl;
        std::cout << "help|h|--h <command>" << std::endl;
        std::cout << "get <userid> <key>" << std::endl;
        std::cout << "set <userid> <key> <value>" << std::endl;
        std::cout << "" << std::endl;
    }

typedef std::vector<std::string> ArgsList;

#pragma region Command Functions
    int HelpCommand(ArgsList &args)
    {
        if (args.size() > 0)
        {
            HelpInfo(args[0]);
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
        return FindValueInAnyUser(key);


        return -1;
    }
    int RemoveUserCommand(ArgsList &args)
    {
        if (args.size() < 1)
        {
            std::cout << "Usage: userdb remove <userid>" << std::endl;
            return -1;
        }
        return RemoveUser(args[0]);
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
    std::string CheckInvite(std::string inviteCode)
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

        if (command == "help" || command == "-help" || command == "--help" || command == "h" || command == "-h")
        { return HelpCommand(args); }
        if (command == "get" || command=="get_user_key")
        { return GetCommand(args);  }
        if (command == "set" || command=="set_user_key")
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
