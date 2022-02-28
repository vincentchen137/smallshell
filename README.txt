By: Vincent Chen, CS 344 - Operating Systems
Portfolio Assignment - smallshell

Instructions on how to compile this code:
1. Go to the working directory of this folder in the command line and type in "gcc --std=gnu99 -o smallshell main.c" .
2. The executable file "smallshell" is now ready to run by typing the following in the command line: "./smallshell" .
3. To run the test script schmod the test script file using "chmod +x ./p3testscript", then type in "./p3testscript 2>&1" from the prompt. 

This program is design to replicate well-known features from bash. It does the following:
1) Provide a prompt for running commands
2) Handle blank lines and comments, which are lines beginning with the # character
3) Execute 3 commands exit, cd, and status via code built into the shell
4) Execute other commands by creating new processes using a function from the exec family of functions
5) Support input and output redirection
6) Support running commands in foreground and background processes