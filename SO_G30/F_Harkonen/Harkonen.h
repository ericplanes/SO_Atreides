#ifndef HARKONEN
#define HARKONEN

// Defines prioritaris
#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 1
#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/wait.h>
#include <math.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <netdb.h>

#include "../F_Extras/Utilities.h"
#include "../F_Extras/Error.h"

// Prints de Harkonen
#define HAR_MSG_INIT "\nStarting Harkonen...\n\n"
#define HAR_EXIT "Exiting...\n\n"
#define HAR_SCAN "Scanning pids...\n"
#define HAR_KILL "Killing pid %d\n\n"
#define HAR_EMPTY "There isn't any pid to kill\n\n"

// Defines del nom del fitxer
#define PS_FILE "F_Harkonen/ps.data"
#define SCAN_FILE "F_Harkonen/scanner.sh"

// Funcions
void HAR_close_program();
void HAR_file_scan(int fd);
void HAR_pid_scan();
int HAR_pid_get();
int HAR_random_position(int max);

// Funcio de print
void print(char *buffer);

#endif