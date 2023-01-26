#ifndef UTILITIES
#define UTILITIES

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

// Defines per les trames origin
#define O_FREMEN "FREMEN"
#define O_ATREIDES "ATREIDES"

// Defines pels tipus d'enviaments i resposta de les trames
#define T_CONNECT 'C'
#define T_CONNECT_OK 'O'
#define T_CONNECT_E 'E'
#define T_DISCONNECT 'Q'
#define T_SEARCH 'S'
#define T_SEARCH_OK 'L'
#define T_SEARCH_E 'K'
#define T_SEND 'F'
#define T_SEND_D 'D'
#define T_SEND_OK 'I'
#define T_SEND_E 'R'
#define T_PHOTO 'P'
#define T_PHOTO_F 'F'
#define T_PHOTO_D 'D'
#define T_PHOTO_OK 'I'
#define T_PHOTO_E 'R'
#define T_UNKNOWN 'Z'

// Defines del contingut dels missatges
#define D_PHOTO_NOT_FOUND "FILE NOT FOUND"

// Altres defines
#define IMAGE_DATA_SIZE 128

// Estructura per fer la comunicacio amb trames
typedef struct
{
    char origen[15];
    char tipus;
    char data[240];
} Trama;

// Funciones generales
Trama fillTramaNoData(char *origen, char tipus);
Trama fillTrama(char *origen, char tipus, char *data);
char *EXTRA_read_line(int fd, char last_char);
char *EXTRA_lower_string(char *string);
int EXTRA_equals_ignore_case(char *first, char *second);
char *EXTRA_copy_string(char *string);
char *EXTRA_substring(char *string, int first, int last);

#endif