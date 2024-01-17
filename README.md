# Description

The implementation of the project includes several aspects such as:
- C programming
- debugging tools such as GDB and valgrind
- Makefile
- proper paths and directories management
- System Calls
- Processes and POSIX Threads management
- Socket Communications (local)
- Signal management: masks and signal handlers
- bash scripts for testing
- more operations

# Makefile – Commands

The Makefile provides several useful commands for compiling, testing and cleaning up the system:
- make all: Compiles and links the targets farm, generafile and collector
- make test: Runs the command "bash test.sh" to start testing using the script file "test.sh"
- make clean: Cleans the system of files generated during compilation and testing

# Binary file generator

generafile.c (filegenerator): Code for generating binary files containing long values (used for testing).

# Additional testcases

Located within the testcases directory.




