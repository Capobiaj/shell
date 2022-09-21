Hi. You can compile the program like this:

gcc -std=c99 -o smallsh smallsh.c

Then run it with this:

./smallsh

Program has the following functionality:

- Provide a prompt for running commands
- Handle blank lines and comments, which are lines beginning with the # character
- Provide expansion for the variable $$
- Execute 3 commands exit, cd, and status via code built into the shell
- Execute other commands by creating new processes using a function from the exec family of functions
- Support input and output redirection
- Support running commands in foreground and background processes
