#ifndef FREMEN
#define FREMEN

// Defines prioritaris
#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 1

// Llibreries generals
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
#include <pthread.h>

// Llibreries propies
#include "../F_Extras/Error.h"
#include "../F_Extras/Utilities.h"

// Prints informatius
#define FR_WELCOME "\nBenvingut a Fremen\n"

// Prints informatius sobre el servidor
#define FR_ATR_DISC "Desconnectat d'Atreides. Dew!\n"
#define FR_ATR_ALREADY_CON "Ja estàs connectat a Atreides\n"
#define FR_ATR_CON_LOST "S'ha perdut la connexio amb el servidor d'Atreides\n"

// Defines de login
#define FR_LIN_WELCOME "Benvingut %s. Tens ID %d\n"
#define FR_LIN_CON "Ara estàs connectat a Atreides.\n"

// Defines del search
#define FR_SRCH_RESP_FORMAT "Hi ha %d persones humanes a %s\n"

// Defines del send
#define FR_SEND_NOT_FOUND "No s'ha trobat la imatge dins la memoria local.\n"
#define FR_SEND_OK "Foto enviada amb exit a Atreides.\n"

// Defines de photo
#define FR_FILE_NOT_FOUND "No hi ha foto registrada.\n"
#define FR_PHOTO_OK "Foto descarregada.\n"
#define FR_PHOTO_CORRUPTED "ERROR: The image has been corrupted.\n"

// Errors propis de la natura del programa
#define FRERR_CODE_SIZE "ERROR: El codi postal ha de tenir obligatoriament 5 digits\n"
#define FRERR_CODE_NUM "ERROR: El codi postal nomes pot contenir nombres\n"
#define FRERR_CODE_SIGN "ERROR: El codi postal ha de ser un nombre positiu de 5 xifres\n"
#define FRERR_NAME_LENGTH "ERROR: El nom no pot ser mes llarg de 20 caracters, no et motivis nano\n"
#define FRERR_ATR_NOT_CON "ERROR: No estas connectat a Atreides\n"
#define FRERR_CON_FAIL "ERROR: No s'ha pogut connectar amb Atreides\n"

// Codi de comandes
#define LOGIN 1
#define SEARCH 2
#define SEND 3
#define PHOTO 4
#define LOGOUT 5
#define LINUX 6

// Nom del fitxer on guardarem els md5sum
#define FILE_MD5SUM "md5sum.data"
#define FILE_LS "ls.data"

// Structs
typedef struct
{
    int time, port;
    char *ip, *path;
} Configuration;

// Structs
typedef struct
{
    int num_param;
    char **command_line;
} Command;

// Variables globals
Command g_com;
Configuration g_config_fremen;
int g_server_fd;

int g_user_id;
char *g_user_name;
char *g_user_code;

// Funcions checkers
int FR_login_check();
int FR_search_check();
int FR_send_check();
int FR_photo_check();
int FR_logout_check();

// Funcions execute
void FR_login_execute();
void FR_search_execute();
void FR_send_execute();
void FR_photo_execute();
void FR_logout_execute();
void FR_com_execute();

// Funcions complementaries de les comandes locals
int FR_send_petition(char *path_image);

// Funcions amb fitxers
void FR_file_md5sum(int fd, char *path);
void FR_file_ls(int fd, char *path);
char *FR_file_getpath(char *file);
int FR_file_length(char *file);
void FR_file_clean();

// Funcions extres de Fremen
void FR_trama_unknown();
void FR_config_read(int argc, char *argv[]);
void FR_config_free();
void FR_server_lost();
void FR_server_disconnect();
void FR_com_read();
void FR_com_free();
int FR_com_getcode();
void FR_fork_clear();

// Print diferent al d'atreides perquè no requereix de semafors
void print(char *buffer);

#endif