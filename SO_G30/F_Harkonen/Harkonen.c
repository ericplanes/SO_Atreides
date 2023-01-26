#include "Harkonen.h"

int main(int argc, char *argv[])
{
    // Variables locals
    int time, pid;
    char buffer[100];

    // Redefinim la funcionalitat del SIGINT per tal de tancar bé el procés en cas d'activació
    signal(SIGINT, HAR_close_program);

    // Mirem que no s'hagin passat d'arguments
    if (argc > 2)
    {
        print(ERROR_PARAM_EXCEEDED);
        return -1;
    }

    // Mirem que no s'hagin quedat curts d'arguments
    if (argc < 2)
    {
        print(ERROR_PARAM_UNREACHED);
        return -1;
    }

    // En cas que ens hagin passat els arguments de manera correcta, guardem el temps
    time = atoi(argv[1]);

    // Informem que hem iniciat el programa
    print(HAR_MSG_INIT);
    while (1)
    {
        // Informem que anem a escanejar els pid
        print(HAR_SCAN);

        // Escanejem els pid
        HAR_pid_scan();

        // Escollim un pid a l'atzar
        pid = HAR_pid_get();

        if (pid > 0)
        {
            sprintf(buffer, HAR_KILL, pid);
            print(buffer);
            kill((pid_t)pid, SIGINT);
        }

        // Esperem el temps que ens han enviat per tal de fer un altre kill
        sleep(time);
    }

    return 0;
}

// Funcio encarregada d'escanejar els pids i guardar-ho en un fitxer
void HAR_pid_scan()
{
    int fd_scan;

    // Obrim el fitxer on guardarem
    fd_scan = open(PS_FILE, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd_scan < 0)
    {
        print(ERROR_FILE_O);
        return;
    }

    // Executem el bash i ho guardem al fitxer
    HAR_file_scan(fd_scan);

    // Tanquem el file descriptor
    close(fd_scan);
}

// Executa un script de bash encarregat d'escanejar
void HAR_file_scan(int fd)
{
    pid_t pid;

    // Carreguem la info de la comanda que volem fer
    char *args[] = {"bash", SCAN_FILE, "Fremen", NULL};

    // Fem el fork i la executem
    switch ((pid = fork()))
    {
    case -1:
        print(ERROR_FORK_CREAT);
        exit(ERROR_CODE);
        break;
    case 0:
        // Redirigim el output del fd 1 (pantalla) al nostre fitxer
        dup2(fd, 1);

        // Executem la comanda
        if (execvp(args[0], args) == -1)
        {
            exit(-1);
        }
        break;
    default:
        wait(NULL);
        break;
    }
}

// Funció encarregada d'escollir un element aleatori del fitxer de pids
int HAR_pid_get()
{
    int fd_scan, *list_pid, i, pid;
    char *line;

    // Obrim el fitxer on estan els pid que hem llegit
    fd_scan = open(PS_FILE, O_RDONLY);
    if (fd_scan < 0)
    {
        print(ERROR_FILE_O);
        return -1;
    }

    // Carreguem una llista amb cada pid
    list_pid = malloc(sizeof(int));
    line = EXTRA_read_line(fd_scan, '\n');
    for (i = 0; (int)strlen(line) > 0; i++)
    {
        // Guardem el pid
        list_pid[i] = atoi(line);

        // Agafem lloc per un pid extra
        list_pid = realloc(list_pid, (i + 2) * sizeof(int));

        // Alliberem la linia actual
        free(line);

        // Llegim la nova linia
        line = EXTRA_read_line(fd_scan, '\n');
    }

    // Alliberem recursos
    free(line);
    close(fd_scan);

    // Escollim un pid a l'atzar, l'ultim pid no l'agafem perquè és els dels dos grep
    if (i < 3)
    {
        print(HAR_EMPTY);
        free(list_pid);
        return -1;
    }
    else
    {
        pid = list_pid[HAR_random_position(i - 2)];
    }

    // Alliberem la memoria
    free(list_pid);

    // Retornem el pid
    return pid;
}

// Retorna un nombre aleatori entre 0 i el valor especificat - 1
int HAR_random_position(int max)
{
    // Posem la seed de random en aleatoria per a que no ens doni sempre el mateix
    srand(time(NULL));

    // Generem un nombre entre 0 i 899 i li sumem 100, pel que el rang esta [100, 99999]
    return (rand() % max);
}

// Funcio encarregada de tancar correctament el programa
void HAR_close_program()
{
    // Manies propies, aixi quan fem control + C amb valgrind crea un espai abans i no surt enganxat
    print("\n\n");
    print(HAR_EXIT);

    // tanquem els file descriptors
    close(0);
    close(1);
    close(2);

    // Tornem el CTRL+C a la funcio original
    signal(SIGINT, SIG_DFL);

    // Ens despedim i tirem un SIGINT per parar l'execucio
    raise(SIGINT);
}

// Funcio encarregada del print d'aquest procés
void print(char *buffer)
{
    write(1, buffer, strlen(buffer));
}