# ssh-import-id
ssh-import-id written in C language

## Contributions

Any PR(s) are welcomed.

## TODO
- ~~add -u flag for useragent~~
- ~~add launchpad provider~~
- ~~add -t flag for truncate (overwrite current keys)~~
- add -o flag for output
- add -r flag for remove
- add ability to fetch multiple userids
- add capability to understand x_ratelimit_remaining header for github

## Known Issues
- Currently, User-Agent option is only supported 16 characters which is too short for most real User-Agents.
