#include "Utilities.h"

// Llegeix una cadena de caracters
char *EXTRA_read_line(int fd, char last_char)
{
    char *string;
    char a_char;
    int i = 0;

    string = malloc(sizeof(char));

    // Bucle per llegir tota la linia
    while (read(fd, &a_char, sizeof(char)) != 0 && a_char != last_char)
    {
        string[i] = a_char;
        string = realloc(string, (i + 2) * sizeof(char));
        i++;
    }

    // Tanquem el string amb un \0
    string[i] = '\0';

    // Retornem el string que hem guardat
    return string;
}

// Compares 2 strings, returns 1 if they are the same (ignorecase)
int EXTRA_equals_ignore_case(char *first, char *second)
{
    int return_value;
    char *lowered_first = EXTRA_lower_string(first);
    char *lowered_second = EXTRA_lower_string(second);

    return_value = strcmp(lowered_first, lowered_second);

    free(lowered_first);
    free(lowered_second);

    if (return_value == 0)
    {
        return 1;
    }

    return 0;
}

// Lowers all the string line
char *EXTRA_lower_string(char *string)
{
    int i;
    char *lowered = malloc(((int)strlen(string) + 1) * sizeof(char));

    for (i = 0; i < (int)strlen(string); i++)
    {
        lowered[i] = tolower(string[i]);
    }

    lowered[i] = '\0';

    return lowered;
}

// Returns a copy of the provided string
char *EXTRA_copy_string(char *string)
{
    int i;
    char *copy;
    copy = malloc(sizeof(char));
    for (i = 0; i < (int)strlen(string); i++)
    {
        copy[i] = string[i];
        copy = realloc(copy, (i + 2) * sizeof(char));
    }
    copy[i] = '\0';
    return copy;
}

// Substrings a string from first to last
char *EXTRA_substring(char *string, int first, int last)
{
    int i, j;
    char *substr;
    substr = malloc(sizeof(char));
    i = first;
    for (j = 0; i <= last; i++, j++)
    {
        substr[j] = string[i];
        substr = realloc(substr, (j + 2) * sizeof(char));
    }
    substr[j] = '\0';
    return substr;
}

// Emplena la trama amb la informacio que se li proporcioni
Trama fillTrama(char *origen, char tipus, char *data)
{
    int i;
    Trama trama;

    // Emplenem la trama origen
    for (i = 0; i < (int)strlen(origen); i++)
    {
        trama.origen[i] = toupper(origen[i]);
    }

    // Emplenem la resta amb \0
    for (; i < 15; i++)
    {
        trama.origen[i] = '\0';
    }

    // Emplenem el tipus
    trama.tipus = toupper(tipus);

    // Emplenem les dades
    for (i = 0; i < (int)strlen(data); i++)
    {
        trama.data[i] = data[i];
    }

    // Emplenem la resta amb \0
    for (; i < 240; i++)
    {
        trama.data[i] = '\0';
    }

    return trama;
}

// Emplena la trama només amb l'origen i el tipus
Trama fillTramaNoData(char *origen, char tipus)
{
    int i;
    Trama trama;

    // Emplenem la trama origen
    for (i = 0; i < (int)strlen(origen); i++)
    {
        trama.origen[i] = toupper(origen[i]);
    }

    // Emplenem la resta amb \0
    for (; i < 15; i++)
    {
        trama.origen[i] = '\0';
    }

    // Emplenem el tipus
    trama.tipus = toupper(tipus);

    return trama;
}