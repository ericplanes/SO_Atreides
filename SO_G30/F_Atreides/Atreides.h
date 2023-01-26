#ifndef ATREIDES
#define ATREIDES

// Defines prioritaris
#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 1
#define _GNU_SOURCE 1

// Llibreries publiques
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

// Llibreries propies
#include "../F_Extras/Utilities.h"
#include "../F_Extras/Error.h"

// Prints per pantalla inicials
#define ATR_MSG_INIT "\nSERVIDOR ATREIDES\n"
#define ATR_MSG_READ_CONF "Llegint el fitxer de configuracio\n"
#define ATR_MSG_WAIT "Esperant connexions...\n\n"

// Prints de la funcio de Login (LIN)
#define ATR_LIN_REC "Rebut login %s %s\n"
#define ATR_LIN_ID_ASIG "Assignat a ID %d\n"
#define ATR_LIN_ANSWERED "Enviada resposta\n\n"

// Prints de la funcio de Search (SRCH)
#define ATR_SRCH_REC "Rebut search %s de %s %d\n"
#define ATR_SRCH_DONE "Feta la cerca\n"
#define ATR_SRCH_RESP_FORMAT "Hi ha %d persones humanes a %s\n"
#define ATR_SRCH_ANSWERED "Enviada resposta\n"

// Prints de la funcio de Logout (LOUT)
#define ATR_LOUT_REC "Rebut logout %s %d\n"
#define ATR_LOUT_DONE "Desconnectat d'Atreides.\n"

// Prints de la funcio de Send (SEND)
#define ATR_SEND_REC "Rebut send %s de %s %d\n"
#define ATR_SEND_DONE "Guardada com %s\n"
#define ATR_SEND_CORRUPTED "La informacio ha estat corrompuda, no s'ha pogut guardar be la imatge.\n"

// Prints de la funcio de Photo (PHOTO)
#define ATR_PHOTO_REC "Rebut photo %d de %s %d\n"
#define ATR_PHOTO_NR "No hi ha foto registrada.\n"
#define ATR_PHOTO_ANSWERED "Enviada resposta\n"
#define ATR_PHOTO_SEND "Enviament %s\n"

// Defines del servidor
#define LISTEN_BACKLOG 64
#define MAX_CONNECTIONS_NUM 10

// Altres defines que necessitarem
#define USERS_DATA_FILE "users.data"
#define MD5SUM_FILE "md5sum.data"

// Estructures que utilitzarem
typedef struct
{
    int id;
    char *name;
    char *code;
} User;

typedef struct
{
    int port;
    char *ip, *path;
} Configuration;

// Variables globals que no modifiquem en cap moment
Configuration g_config;
int g_fd_server;

// Variables globals que si modifiquem
User *g_list_users;
int g_num_users;
int *g_list_fd_clients;
pthread_t *g_list_threads;
int g_num_clients;
pthread_attr_t detached_attr;

// Mutex per sincronitzacions
static pthread_mutex_t mtx_print = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_user_list = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_images = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_md5sum = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_logout = PTHREAD_MUTEX_INITIALIZER;

// Funcions de configuracio de servidor
void ATR_read_conf(int argc, char *argv[]);
void ATR_read_usersfile();
int ATR_server_launch();
void ATR_server_close();

// Funcions complementaries al servidor
void ATR_free_userlist();
void ATR_free_configfile();
void ATR_close_socketsfd();
void ATR_cancel_threads();

// Funcions propies per als threads
static void *ATR_server_runclient(void *arg);
User ATR_client_login(int fd_client);
void ATR_client_search(Trama trama, int fd_client);
void ATR_client_send(User user, Trama trama, int fd_client);
void ATR_client_photo(User user, Trama trama, int fd_client);
void ATR_client_logout(User user, int fd_client);

// Funcions complementaries dels threads
void ATR_file_md5sum(int fd, char *path);
void ATR_file_register_user(User user);
void ATR_file_modify_user(User user, char *actual);
int ATR_file_length(char *file);
char *ATR_file_getpath(char *file);
char *ATR_format_user(User user);
int ATR_format_random_id();
void ATR_trama_unknown(int fd_client);

// Funcions pels semafors dels threads
void ATR_mutex_lock(pthread_mutex_t mutex);
void ATR_mutex_unlock(pthread_mutex_t mutex);

// Extra
void print(char *buffer);

#endif