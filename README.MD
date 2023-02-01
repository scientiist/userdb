# UserDB v1
### A scuffed JSON database written in c++

## Why?
I needed a fast way to pull user info to and from a "database" file, which would interface nicely with bash and php.

## Commands

#### Help Command

`userdb help|-h` Shows list of all commands.

`userdb help|-h <command>` Displays info for specific command.
#### Get Command
`userdb get|get_user_key <userid> <key>`  Retrieves a user value.
#### Set Command
`userdb set|set_user_key <userid> <key> <value>` Stores a user value.

Value will be evaluated as a json literal if possible.
#### Remove User Command
`userdb remove_user <userid>` Removes a user entry from the database.
#### Remove User Key Command
`userdb remove_user_key <userid> <key>`  Removes a key from a user entry.
#### Add User Command
`userdb add_user <userid>` Add a user entry to the database.
#### Username Taken Command
`userdb is_username_taken <userid>`  Checks if username is already in use.
#### Dump User Entry Command
`userdb dump <userid>` Dumps a user entry as raw JSON.
#### Search Key Command
`userdb search_k <key>` Recursive searches the entire database for a specified key.
#### Search Value Command
`userdb search_v <value>` Recursive searches the entire database for a specified value.
#### Check Invite Command
`userdb checkinv <invitecode>` Checks a user invite code for validity: Returns the owner of the code.
#### Use Invite Command
`userdb useinvite <invitecode>` Authenticates and consumes a user invite code. Returns the owner of the code.
#### Count Command
`userdb count <keyword>` counts instances of a specified keyword throughout the database

