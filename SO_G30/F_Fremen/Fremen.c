#include "Fremen.h"

int main(int argc, char *argv[])
{
    // Variables del main
    int com_code;

    // Configurem CTRL+C per a que passi per la funcio executeLogout abans de sortir
    signal(SIGINT, FR_logout_execute);

    // Configurem el SIGPIPE per tal de saber si perdem la connexio amb el servidor
    signal(SIGPIPE, FR_server_lost);

    // Congigurem SIGALRM per tal d'anar eliminant els fitxers locals que no siguin propis
    signal(SIGALRM, FR_file_clean);

    // Carreguem el fitxer de configuration
    FR_config_read(argc, argv);

    // Fiquem el fd del servidor a -1 per saber que encara no estem connectats al servidor
    g_server_fd = -1;

    // Fem un raise de SIGALRM amb el temps configurat per tal d'eliminar els fitxers descarregats
    alarm(g_config_fremen.time);

    // Iniciem el programa
    print(FR_WELCOME);
    while (1)
    {
        // Llegim la comanda
        do
        {
            // Esperem a que l'usuari escrigui algo
            FR_com_read();

            if (g_com.num_param < 1)
            {
                FR_com_free();
            }
        } while (g_com.num_param < 1);

        // Obtenim el codi de la comanda
        com_code = FR_com_getcode(g_com.command_line[0]);

        // Mirem quina comanda s'ha introduit i executem el codi pertinent
        switch (com_code)
        {
        case LOGIN:
            if (FR_login_check())
            {
                FR_login_execute();
            }
            break;
        case SEARCH:
            if (FR_search_check())
            {
                FR_search_execute();
            }
            break;
        case SEND:
            if (FR_send_check())
            {
                FR_send_execute();
            }
            break;
        case PHOTO:
            if (FR_photo_check())
            {
                FR_photo_execute();
            }
            break;
        case LOGOUT:
            if (FR_logout_check())
            {
                raise(SIGINT);
            }
            break;
        case LINUX:
            FR_com_execute();
            break;
        default:
            print(ERROR_COM_NOT_FOUND);
            break;
        }

        // Alliberem la memoria de la comanda
        FR_com_free();
    }

    return 0;
}

// Confirma que tot s'hagi enviat correctament i retorna un struct de configuració ple
void FR_config_read(int argc, char *argv[])
{
    int fd;
    char *line;

    // Comprovem que ens hagin passat un paràmetre correcte
    if (argc != 2)
    {
        if (argc < 2)
        {
            print(ERROR_CONF_MISSING);
            exit(ERROR_CODE);
        }
        if (argc > 2)
        {
            print(ERROR_PARAM_EXCEEDED);
            exit(ERROR_CODE);
        }
    }
    // Mirem si el document existeix, si existeix, emplenem config
    else
    {
        fd = open(argv[1], O_RDONLY);
        if (fd == -1)
        {
            print(ERROR_CONF_UNEXISTENT);
            exit(ERROR_CODE);
        }

        line = EXTRA_read_line(fd, '\n');
        g_config_fremen.time = atoi(line);
        free(line);

        line = EXTRA_read_line(fd, '\n');
        g_config_fremen.ip = line;

        line = EXTRA_read_line(fd, '\n');
        g_config_fremen.port = atoi(line);
        free(line);

        line = EXTRA_read_line(fd, '\n');
        g_config_fremen.path = line;

        close(fd);
    }
}

// Llegeix i emplena el struct amb la comanda que escrigui l'usuari
void FR_com_read()
{
    int length = 1, i, pos;
    char buffer[500];

    // Inicialitzem els parametres de la comanda i reservem memoria
    g_com.num_param = 0;
    g_com.command_line = malloc(sizeof(char *));
    g_com.command_line[0] = malloc(sizeof(char));

    // Llegir una comanda (Si és buida, es queda en bucle)
    do
    {

        print("$ ");
        length = read(0, buffer, 500);

    } while (length <= 1);

    // Descomposem la comanda i ho guardem en un struct Comanda
    for (i = 0, pos = 0; buffer[i] != '\n'; i++)
    {
        // Si és un espai, ho tractem
        if (buffer[i] == ' ')
        {
            // Buidem els espais que hi hagi entre parametres
            while (buffer[i] == ' ')
            {
                // Si son els espais del final de la comanda, sortim
                if (buffer[i + 1] == '\n')
                {
                    return;
                }
                i++;
            }
            i--;

            // Si els espais estaven a l'inici de la comanda no fem res
            if (g_com.num_param == 0)
            {
                g_com.num_param++;
            }

            // Preparem tot per passar a llegir el seguent parametre
            else
            {
                // Preparem el seguent parametre
                g_com.num_param++;
                g_com.command_line = realloc(g_com.command_line, g_com.num_param * sizeof(char *));
                g_com.command_line[g_com.num_param - 1] = malloc(sizeof(char));
                pos = 0;
            }
        }
        else
        {
            //  Si encara no hem emplenat cap parametre, incrementem el nombre de parametres
            if (g_com.num_param == 0)
            {
                g_com.num_param++;
            }

            // Emplenem la posicio del caracter al parametre
            g_com.command_line[g_com.num_param - 1][pos] = buffer[i];
            g_com.command_line[g_com.num_param - 1] = realloc(g_com.command_line[g_com.num_param - 1], (pos + 2) * sizeof(char));
            pos++;

            // Si es l'ultim caracter del parametre, tanquem el string
            if (buffer[i + 1] == ' ' || buffer[i + 1] == '\n')
            {
                g_com.command_line[g_com.num_param - 1][pos] = '\0';
            }
        }
    }
}

// Funcio activada per SIGALRM que utilitzarem per netejar els fitxers cada X segons (utilitzant el que ens hagin enviat per configuració)
void FR_file_clean()
{
    // Variables locals
    int fd_ls, num_removed;
    char *path_ls, *format, *line, *aux, buffer[50];

    // Guardem el path on trobarem ls
    path_ls = FR_file_getpath(FILE_LS);

    // Escribim al fitxer ls la informacio del ls
    fd_ls = open(path_ls, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd_ls < 0)
    {
        free(path_ls);
        print(ERROR_FILE_O);
        return;
    }

    // Executem ls i ho guardem al fitxer
    sprintf(buffer, "%s/", g_config_fremen.path);
    FR_file_ls(fd_ls, buffer);

    // Tanquem el fitxer en format d'escriptura
    close(fd_ls);

    // Llegim la info del ls
    fd_ls = open(path_ls, O_RDONLY);
    if (fd_ls < 0)
    {
        free(path_ls);
        print(ERROR_FILE_O);
        return;
    }

    // Alliberem la memoria del path, ja que no la utilitzarem més
    free(path_ls);

    // Llegim la informacio del fitxer LS i anem eliminant els arxius corresponents
    num_removed = 0;
    line = EXTRA_read_line(fd_ls, '\n');
    while ((int)strlen(line) > 0)
    {
        // Primer de tot mirem que sigui suficientment llarg com per poder ser .jpg
        if ((int)strlen(line) > 4)
        {
            // Mirem que sigui un .jpg
            format = EXTRA_substring(line, (int)strlen(line) - 4, (int)strlen(line));
            if (EXTRA_equals_ignore_case(format, ".jpg"))
            {
                // Mirem que el nom sigui un numero (que voldria dir que s'ha descarregat del servidor)
                aux = EXTRA_substring(line, 0, (int)strlen(line) - 5);
                if (atoi(aux) > 0)
                {
                    // Eliminem l'arxiu
                    sprintf(buffer, "%s/%s", g_config_fremen.path, line);
                    if (remove(buffer) == 0)
                    {
                        num_removed++;
                    }
                }
                free(aux);
            }

            // Alliberem la memoria que hem utilitzat per fer el format
            free(format);
        }

        // Alliberem la memoria utilitzada per llegir la linia
        free(line);

        // Llegim la seguent linia
        line = EXTRA_read_line(fd_ls, ' ');
    }

    // Informem del nombre de fitxers que hem eliminat
    sprintf(buffer, "S'han netejat %d fitxers.\n", num_removed);
    print(buffer);

    // Alliberem la memoria utilitzada per llegir la linia
    free(line);

    // Tanquem el fd
    close(fd_ls);

    // Congigurem l'alarm per tal d'anar eliminant els fitxers locals que no siguin propis
    signal(SIGALRM, FR_file_clean);

    // Executem un altre alarm
    alarm(g_config_fremen.time);
}

// Comproba que la comanda Login tingui els parametres corresponents
int FR_login_check()
{
    if (g_com.num_param > 3)
    {
        print(ERROR_PARAM_EXCEEDED);
        return 0;
    }
    if (g_com.num_param < 3)
    {
        print(ERROR_PARAM_UNREACHED);
        return 0;
    }
    if (g_server_fd != -1)
    {
        print(FR_ATR_ALREADY_CON);
        return 0;
    }
    if (strlen(g_com.command_line[2]) != 5)
    {
        print(FRERR_CODE_SIZE);
        return 0;
    }
    else
    {
        for (int i = 0; i < (int)strlen(g_com.command_line[2]); i++)
        {
            if (!isdigit(g_com.command_line[2][i]))
            {
                print(FRERR_CODE_NUM);
                return 0;
            }
        }
    }

    if (strlen(g_com.command_line[1]) > 20)
    {
        print(FRERR_NAME_LENGTH);
        return 0;
    }
    return 1;
}

// Comproba que la comanda Logout tingui els parametres corresponents
int FR_search_check()
{
    if (g_com.num_param > 2)
    {
        print(ERROR_PARAM_EXCEEDED);
        return 0;
    }

    if (g_com.num_param < 2)
    {
        print(ERROR_PARAM_UNREACHED);
        return 0;
    }

    if (strlen(g_com.command_line[1]) != 5)
    {
        print(FRERR_CODE_SIZE);
        return 0;
    }

    if (g_server_fd < 0)
    {
        print(FRERR_ATR_NOT_CON);
        return 0;
    }

    if (strlen(g_com.command_line[1]) != 5)
    {
        print(FRERR_CODE_SIZE);
        return 0;
    }
    else
    {
        for (int i = 0; i < (int)strlen(g_com.command_line[1]); i++)
        {
            if (!isdigit(g_com.command_line[1][i]))
            {
                print(FRERR_CODE_NUM);
                return 0;
            }
        }
    }

    return 1;
}

// Comproba que la comanda Send tingui els parametres corresponents
int FR_send_check()
{
    char *format;

    if (g_com.num_param > 2)
    {
        print(ERROR_PARAM_EXCEEDED);
        return 0;
    }
    if (g_com.num_param < 2)
    {
        print(ERROR_PARAM_UNREACHED);
        return 0;
    }
    if (strlen(g_com.command_line[1]) > 30)
    {
        print(ERROR_LENGTH_EXCEEDED);
        return 0;
    }

    // Mirem que l'arxiu tingui una mida minima de 5 (caracter + .jpg)
    if ((int)strlen(g_com.command_line[1]) < 5)
    {
        print(ERROR_FORMAT_IMAGE);
        return 0;
    }

    // Mirem que l'arxiu a enviar sigui .jpg
    format = EXTRA_substring(g_com.command_line[1], (int)strlen(g_com.command_line[1]) - 5, (int)strlen(g_com.command_line[1]) - 1);
    if (EXTRA_equals_ignore_case(format, ".jpg"))
    {
        print(ERROR_FORMAT_IMAGE);
        free(format);
        return 0;
    }
    free(format);
    return 1;
}

// Comproba que la comanda Photo tingui els parametres corresponents
int FR_photo_check()
{
    if (g_com.num_param > 2)
    {
        print(ERROR_PARAM_EXCEEDED);
        return 0;
    }
    if (g_com.num_param < 2)
    {
        print(ERROR_PARAM_UNREACHED);
        return 0;
    }
    return 1;
}

// Comproba que la comanda Logout tingui els parametres corresponents
int FR_logout_check()
{
    if (g_com.num_param > 1)
    {
        print(ERROR_PARAM_EXCEEDED);
        return 0;
    }
    if (g_com.num_param < 1)
    {
        print(ERROR_PARAM_UNREACHED);
        return 0;
    }
    return 1;
}

// Connecta el client de Fremen amb el servidor Atreides
void FR_login_execute()
{
    // Preparem les trames pel login
    Trama send_trama, rec_trama;
    char buffer[100];

    // Primer de tot, mirem que no haguem fet ja login
    if (g_server_fd >= 0)
    {
        print(FR_ATR_ALREADY_CON);
        return;
    }

    // Comprovem que el port sigui valid
    if (g_config_fremen.port < 1025 || g_config_fremen.port > 65535)
    {
        print(CERR_INV_PORT);
        return;
    }

    // Obrim el fd del socket del client amb ipv4 i TCP
    g_server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_server_fd < 0)
    {
        print(CERR_SOCKET_CREAT);
        return;
    }

    // Especifiquem on ens connectarem
    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof(s_addr)); // Posa tota l'estructura a 0
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(g_config_fremen.port);

    // Guardem la IP per fer la connexio
    struct in_addr ip;
    if (inet_pton(AF_INET, g_config_fremen.ip, &ip) < 0)
    {
        print(CERR_IP);
        close(g_server_fd);
        return;
    }

    // Si tot ha anat bé, guardarem la IP i intentarem connectar-nos amb el servidor
    s_addr.sin_addr = ip;
    if (connect(g_server_fd, (void *)&s_addr, sizeof(s_addr)) < 0)
    {
        print(FRERR_CON_FAIL);
        close(g_server_fd);
        g_server_fd = -1;
        return;
    }

    // Li enviem al servidor una peticio de connexio amb les nostres dades
    sprintf(buffer, "%s*%s", g_com.command_line[1], g_com.command_line[2]);
    send_trama = fillTrama(O_FREMEN, T_CONNECT, buffer);
    write(g_server_fd, &send_trama, sizeof(Trama));

    // Esperem la resposta del servidor
    read(g_server_fd, &rec_trama, sizeof(Trama));

    // Revisem la resposta del servidor
    if (rec_trama.tipus == T_CONNECT_E)
    {
        print(FRERR_CON_FAIL);
        return;
    }

    if (rec_trama.tipus == T_CONNECT_OK)
    {
        // Guardem els va lors de la connexió en variables globals
        g_user_id = atoi(rec_trama.data);
        g_user_name = EXTRA_copy_string(g_com.command_line[1]);
        g_user_code = EXTRA_copy_string(g_com.command_line[2]);

        // Informem que hem sigut connectats
        sprintf(buffer, FR_LIN_WELCOME, g_user_name, g_user_id);
        print(buffer);
        print(FR_LIN_CON);
        return;
    }

    if (rec_trama.tipus == T_UNKNOWN)
    {
        print(CERR_UNKNOWN_SEND);
        return;
    }

    FR_trama_unknown();
    print(CERR_UNKNOWN_RESP);
}

// Executa la comanda search
void FR_search_execute()
{
    int num_of_rec, i, j, pos, num_persones, part, connection;
    char buffer[100], *string;
    Trama send_trama, *rec_trama;

    // Mirem que estiguem connectats al servidor abans de res
    if (g_server_fd < 0)
    {
        print(CERR_NOT_CON);
        return;
    }

    // Preparem i enviem la petició de search
    sprintf(buffer, "%s*%d*%s", g_user_name, g_user_id, g_com.command_line[1]);
    send_trama = fillTrama(O_FREMEN, T_SEARCH, buffer);
    write(g_server_fd, &send_trama, sizeof(Trama));

    // Rebem la primera trama
    rec_trama = malloc(sizeof(Trama));

    // Llegim la resposta del servidor
    connection = read(g_server_fd, &rec_trama[0], sizeof(Trama));

    // Mirem que el read s'hagi fet be
    if (connection <= 0)
    {
        FR_server_lost();
        free(rec_trama);
        return;
    }

    // Mirem si hi ha hagut algun error a la trama que hem enviat
    if (rec_trama[0].tipus == T_SEARCH_E)
    {
        free(rec_trama);
        print(CERR_SEARCH_FORM);
        return;
    }

    if (rec_trama[0].tipus == T_UNKNOWN)
    {
        free(rec_trama);
        print(CERR_UNKNOWN_SEND);
        return;
    }

    if (rec_trama[0].tipus != T_SEARCH_OK)
    {
        free(rec_trama);
        FR_trama_unknown();
        print(CERR_UNKNOWN_RESP);
        return;
    }

    // Mirem el nombre de trames que ens han dit que ens enviaran
    string = malloc(sizeof(char));
    for (i = 0; rec_trama[0].data[i] != '*'; i++)
    {
        string[i] = rec_trama[0].data[i];
        string = realloc(string, (i + 2) * sizeof(char));

        // Mirem que no hi hagi hagut un error en l'enviament i ens estiguem passant
        if (i > 10)
        {
            print(CERR_SEARCH_RESP);
            free(string);
            free(rec_trama);
            return;
        }
    }

    // Tanquem i alliberem el string despres de guardar-nos el nombre de trames que rebrem
    string[i] = '\0';
    num_of_rec = atoi(string);
    free(string);
    i++;

    // Emplenem el string amb les dades restants de la primera trama
    part = 0;
    num_persones = 0;
    string = malloc(sizeof(char));
    for (j = 0; rec_trama[0].data[i] != '\0'; j++, i++)
    {
        // Mirem si hem acabat de llegir un usuari sencer i tractem el string
        if (rec_trama[0].data[i] == '*')
        {
            part++;
            if (part % 2 == 0)
            {
                string[j] = '\n';
            }
            else
            {
                string[j] = ' ';
                num_persones++;
            }
        }
        // Si encara no hem acabat amb la paraula, seguim
        else
        {
            string[j] = rec_trama[0].data[i];
        }
        string = realloc(string, (j + 2) * sizeof(char));
    }

    // Tanquem el string
    string[j] = '\n';
    string = realloc(string, (j + 2) * sizeof(char));

    // Guardem la posicio del string on ens hem quedat
    pos = j + 1;

    // Descomposem les dades de les trames que ens han enviat i les afegim a un string que printarem
    for (i = 1; i < num_of_rec; i++)
    {
        // Rebem la trama
        rec_trama = realloc(rec_trama, (i + 1) * sizeof(Trama));
        read(g_server_fd, &rec_trama[i], sizeof(Trama));

        // Mirem que sigui correcte
        if (rec_trama[i].tipus == T_SEARCH_E)
        {
            free(rec_trama);
            free(string);
            print(CERR_SEARCH_FORM);
            return;
        }

        if (rec_trama[i].tipus == T_UNKNOWN)
        {
            free(rec_trama);
            free(string);
            print(CERR_UNKNOWN_SEND);
            return;
        }

        if (rec_trama[i].tipus != T_SEARCH_OK)
        {
            free(rec_trama);
            free(string);
            FR_trama_unknown();
            print(CERR_UNKNOWN_RESP);
            return;
        }

        // Tractem la trama
        for (j = 0, part = 0; rec_trama[i].data[j] != '\0'; j++)
        {
            // Mirem si hem acabat de llegir un usuari sencer i tractem el string
            if (rec_trama[i].data[j] == '*')
            {
                part++;
                if (part % 2 == 0)
                {
                    string[pos] = '\n';
                }
                else
                {
                    string[pos] = ' ';
                    num_persones++;
                }
            }
            // Si encara no hem acabat amb la paraula, seguim
            else
            {
                string[pos] = rec_trama[i].data[j];
            }
            string = realloc(string, (pos + 2) * sizeof(char));
            pos++;
        }
        string[pos] = '\n';
        string = realloc(string, (pos + 2) * sizeof(char));
        pos++;
    }

    // Tanquem el string
    string[pos - 1] = '\0';
    string[pos] = '\0';

    // Printem tota la informació per pantalla
    sprintf(buffer, FR_SRCH_RESP_FORMAT, num_persones, g_com.command_line[1]);
    print(buffer);
    print(string);

    // Fem tots els frees corresponents
    free(string);
    free(rec_trama);
}

// Executa la comanda send
void FR_send_execute()
{
    // Mirem que estiguem connectats al servidor abans de res
    if (g_server_fd < 0)
    {
        print(CERR_NOT_CON);
        return;
    }

    // Declaracio de variables locals
    Trama send_trama, rec_trama;
    int length, fd_image, i;
    char *name_image, *path_image;

    // Inicialitzem les variables
    name_image = g_com.command_line[1];
    path_image = FR_file_getpath(name_image);

    // Si estem connectats, informem al servidor que farem un send enviant una trama amb nom*mida*md5sum
    length = FR_send_petition(path_image);

    // Comprovem que no hi ha hagut cap problema
    if (length < 0)
    {
        free(path_image);
        return;
    }

    // Obrim el fitxer
    fd_image = open(path_image, O_RDONLY);
    if (fd_image < 0)
    {
        free(path_image);
        print(ERROR_FILE_O);
        return;
    }

    // Inicialitzem la trama que volem enviar
    send_trama = fillTramaNoData(O_FREMEN, T_SEND_D);

    // Enviem la info del fitxer per trames fins que acabem el fitxer
    while (length > IMAGE_DATA_SIZE)
    {
        // Per cada trama, emplenem les dades que hem decidit
        read(fd_image, &send_trama.data, IMAGE_DATA_SIZE * sizeof(char));

        // Emplenem la trama que queda amb '\0'
        for (i = IMAGE_DATA_SIZE; i < 240; i++)
        {
            send_trama.data[i] = '\0';
        }

        // Enviem la trama
        write(g_server_fd, &send_trama, sizeof(Trama));

        // Reconfigurem el valor de la mida que queda per enviar
        length -= IMAGE_DATA_SIZE;
    }

    // Si queda info a enviar, enviem
    if (length > 0)
    {
        // Inicialitzem la trama que volem enviar
        send_trama = fillTramaNoData(O_FREMEN, T_SEND_D);

        // Per cada trama, emplenem les dades que poguem
        read(fd_image, &send_trama.data, length * sizeof(char));

        // Emplenem la trama que queda amb '\0'
        for (i = length; i < 240; i++)
        {
            send_trama.data[i] = '\0';
        }

        // Enviem la trama
        write(g_server_fd, &send_trama, sizeof(Trama));
    }

    // Tanquem el fitxer i alliberem la memoria del path
    close(fd_image);
    free(path_image);

    // Rebem la resposta del thread
    read(g_server_fd, &rec_trama, sizeof(Trama));

    // Comprobem que sigui correcte
    if (rec_trama.tipus == T_SEND_E)
    {
        print(CERR_SEND_RESP);
        return;
    }

    if (rec_trama.tipus == T_UNKNOWN)
    {
        print(CERR_UNKNOWN_SEND);
        return;
    }

    if (rec_trama.tipus != T_SEND_OK)
    {
        FR_trama_unknown();
        print(CERR_UNKNOWN_RESP);
        return;
    }

    // Informem que la foto s'ha enviat correctament
    print(FR_SEND_OK);
}

// Informa al servidor que farà un send i li envia la primera trama amb la informacio corresponent a la imatge
int FR_send_petition(char *path_image)
{
    // Declaracio de variables locals
    Trama send_trama;
    char *path_md5sum, *md5sum, buffer[240];
    int fd_md5sum, length, i;

    // Mirem la mida del fitxer de la imatge
    length = FR_file_length(path_image);
    if (length < 0)
    {
        print(FR_SEND_NOT_FOUND);
        return -1;
    }

    // Guardem la direccio del md5sum
    path_md5sum = FR_file_getpath(FILE_MD5SUM);

    // Creem el fitxer on guardarem el md5sum de la imatge, si ja existeix, el sobreescribim
    fd_md5sum = open(path_md5sum, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd_md5sum < 0)
    {
        free(path_md5sum);
        print(ERROR_FILE_O);
        return -1;
    }

    // Executem el md5sum i ho guardem al fitxer corresponent
    FR_file_md5sum(fd_md5sum, path_image);

    // Tanquem el fitxer
    close(fd_md5sum);

    // Llegim el contingut de md5sum
    fd_md5sum = open(path_md5sum, O_RDONLY);
    if (fd_md5sum < 0)
    {
        free(path_md5sum);
        print(ERROR_FILE_O);
        return -1;
    }

    md5sum = EXTRA_read_line(fd_md5sum, ' ');
    close(fd_md5sum);

    // Comprovem que la linia compleixi les condicions minimes per a ser llegida
    if ((int)strlen(md5sum) != 32)
    {
        free(path_md5sum);
        free(md5sum);
        print(ERROR_MD5SUM);
        return -1;
    }

    for (i = 0; i < 32; i++)
    {
        if (md5sum[i] == ' ')
        {
            free(path_md5sum);
            free(md5sum);
            print(ERROR_MD5SUM);
            return -1;
        }
    }

    // Enviem la peticio de send
    sprintf(buffer, "%s*%d*%s", g_com.command_line[1], length, md5sum);
    send_trama = fillTrama(O_FREMEN, T_SEND, buffer);
    write(g_server_fd, &send_trama, sizeof(Trama));

    // Alliberem tota la memoria utilitzada
    free(path_md5sum);
    free(md5sum);

    // Retornem la mida del fitxer
    return length;
}

// Executa la comanda photo
void FR_photo_execute()
{
    // Mirem que estiguem connectats al servidor abans de res
    if (g_server_fd < 0)
    {
        print(CERR_NOT_CON);
        return;
    }

    // Declaracio de variables locals
    Trama send_trama, rec_trama;
    char *file_name, *md5sum, *aux, *path_image, *path_md5sum;
    int size, i, j, fd_image, fd_md5sum;

    // Peticio de foto a llegir
    send_trama = fillTrama(O_FREMEN, T_PHOTO, g_com.command_line[1]);
    write(g_server_fd, &send_trama, sizeof(Trama));

    // Esperem a rebre una trama del servidor
    read(g_server_fd, &rec_trama, sizeof(Trama));

    // Mirem si la foto és del tipus que necessitem
    if (rec_trama.tipus == T_UNKNOWN)
    {
        print(CERR_UNKNOWN_SEND);
        return;
    }

    if (rec_trama.tipus == T_PHOTO_E)
    {
        print(rec_trama.data);
        return;
    }

    if (rec_trama.tipus != T_PHOTO_F)
    {
        FR_trama_unknown();
        print(CERR_UNKNOWN_RESP);
        return;
    }

    // Mirem si s'ha trobat el fitxer
    if (EXTRA_equals_ignore_case(D_PHOTO_NOT_FOUND, rec_trama.data))
    {
        print(FR_FILE_NOT_FOUND);
        return;
    }

    // Parsejem el nom de la imatge enviada
    file_name = malloc(sizeof(char));
    for (i = 0, j = 0; rec_trama.data[i] != '*'; i++, j++)
    {
        file_name[j] = rec_trama.data[i];
        file_name = realloc(file_name, (j + 2) * sizeof(char));
        if (i > 30)
        {
            print(ERROR_LENGTH_EXCEEDED);
            free(file_name);
            return;
        }
    }
    file_name[j] = '\0';
    i++;

    // Parsejem la mida de la imatge enviada
    aux = malloc(sizeof(char));
    for (j = 0; rec_trama.data[i] != '*'; i++, j++)
    {
        aux[j] = rec_trama.data[i];
        aux = realloc(aux, (j + 2) * sizeof(char));
    }
    aux[j] = '\0';
    i++;

    // Afegim el resultat a la mida i alliberem la memoria
    size = atoi(aux);
    free(aux);

    // Llegim la informació que quedi restant, que serà md5sum
    md5sum = EXTRA_substring(rec_trama.data, i, strlen(rec_trama.data) - 1);

    // Guardem el path que tindra la nostre imatge i alliberem el nom
    path_image = FR_file_getpath(file_name);
    free(file_name);

    // Esborrem el contingut si hi ha algun creat i el tornem a crear
    fd_image = open(path_image, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd_image < 0)
    {
        free(md5sum);
        free(path_image);
        print(ERROR_FILE_O);
        return;
    }

    // Rebem totes les trames alhora que emplenem el fitxer de la imatge amb la info que rebem
    while (size > IMAGE_DATA_SIZE)
    {
        // Llegim la trama
        read(g_server_fd, &rec_trama, sizeof(Trama));

        // Mirem que sigui del tipus correcte
        if (rec_trama.tipus == T_UNKNOWN)
        {
            free(md5sum);
            free(path_image);
            close(fd_image);
            print(CERR_UNKNOWN_SEND);
            return;
        }

        if (rec_trama.tipus != T_PHOTO_D)
        {
            FR_trama_unknown();
            print(CERR_UNKNOWN_RESP);
            free(md5sum);
            free(path_image);
            close(fd_image);
            return;
        }

        // Escribim la informacio
        write(fd_image, &rec_trama.data, IMAGE_DATA_SIZE * sizeof(char));

        // Actualitzem el valor de size
        size -= IMAGE_DATA_SIZE;
    }

    // Rebem i escribim la ultima trama
    if (size > 0)
    {
        // Llegim la trama
        read(g_server_fd, &rec_trama, sizeof(Trama));

        // Mirem que sigui del tipus correcte
        if (rec_trama.tipus == T_UNKNOWN)
        {
            print(CERR_UNKNOWN_SEND);
            free(md5sum);
            free(path_image);
            close(fd_image);
            return;
        }
        if (rec_trama.tipus != T_PHOTO_D)
        {
            FR_trama_unknown();
            print(CERR_UNKNOWN_RESP);
            free(md5sum);
            free(path_image);
            close(fd_image);
            return;
        }

        // Escribim la informacio
        write(fd_image, &rec_trama.data, size * sizeof(char));
    }

    // Tanquem el fd de la imatge
    close(fd_image);

    // Guardem el path on trobarem md5sum
    path_md5sum = FR_file_getpath(FILE_MD5SUM);

    // Escribim al fitxer md5sum la informacio del md5sum
    fd_md5sum = open(path_md5sum, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd_md5sum < 0)
    {
        free(md5sum);
        free(path_image);
        free(path_md5sum);
        print(ERROR_FILE_O);
        return;
    }
    FR_file_md5sum(fd_md5sum, path_image);

    // Tanquem el fitxer en format d'escriptura
    close(fd_md5sum);

    // Llegim la info del md5sum
    fd_md5sum = open(path_md5sum, O_RDONLY);
    if (fd_md5sum < 0)
    {
        free(md5sum);
        free(path_image);
        free(path_md5sum);
        print(ERROR_FILE_O);
        return;
    }

    // Llegim la primera linia i agafem el md5sum del fitxer
    aux = EXTRA_read_line(fd_md5sum, ' ');

    // Tanquem el fd
    close(fd_md5sum);

    // Mirem si el md5sum actual es el mateix que el que hauriem de tenir
    if (strcmp(md5sum, aux) != 0)
    {
        // Enviem una trama informant que ha anat malament
        send_trama = fillTrama(O_FREMEN, T_PHOTO_E, "IMAGE KO");
        write(g_server_fd, &send_trama, sizeof(Trama));

        // Informem que les dades han estat contaminades
        print(FR_PHOTO_CORRUPTED);

        // Alliberem la memoria abans de sortir
        free(md5sum);
        free(path_image);
        free(path_md5sum);
        free(aux);
        return;
    }

    // Si tot ha anat bé, notifiquem a fremen i ho printem per pantalla
    send_trama = fillTrama(O_FREMEN, T_PHOTO_OK, "IMAGE OK");
    write(g_server_fd, &send_trama, sizeof(Trama));
    print(FR_PHOTO_OK);

    // Fem els frees corresponents i sortim
    free(md5sum);
    free(path_image);
    free(path_md5sum);
    free(aux);
}

// Executa la comanda logout, buidant la memoria, tancant fd i forçant un SIGINT
void FR_logout_execute()
{
    // Manies propies, aixi quan fem control + C amb valgrind crea un espai abans i no surt enganxat
    print("\n\n");

    // Ens desconnectem del servidor si hi estem connectats
    if (g_server_fd >= 0)
    {
        FR_server_disconnect();
    }

    // alliberem la memoria que queda pendent
    FR_com_free();
    FR_config_free();

    // tanquem els file descriptors
    close(0);
    close(1);
    close(2);

    // Tornem el CTRL+C a la funcio original
    signal(SIGINT, SIG_DFL);

    // Ens despedim i tirem un SIGINT per parar l'execucio
    raise(SIGINT);
}

// Executa una comanda que no sigui propia
void FR_com_execute()
{
    char *args[20];
    pid_t pid;
    int i;

    for (i = 0; i < g_com.num_param; i++)
    {
        args[i] = g_com.command_line[i];
    }
    args[i] = NULL;

    switch ((pid = fork()))
    {
    case -1:
        print(ERROR_FORK_CREAT);
        exit(ERROR_CODE);
        break;
    case 0:
        // Fem l'execucio
        if (execvp(g_com.command_line[0], args) == -1)
        {
            FR_fork_clear();
        }
        break;
    default:
        wait(NULL);
        break;
    }
}

// Executa md5sum del path que li enviem i ho guarda al fitxer md5sum.data
void FR_file_md5sum(int fd, char *path)
{
    pid_t pid;

    // Carreguem la info de la comanda que volem fer
    char *args[] = {"md5sum", path, NULL};

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

        // Fem l'execucio
        if (execvp(args[0], args) == -1)
        {
            FR_fork_clear();
        }
        break;
    default:
        wait(NULL);
        break;
    }
}

// Executa ls del path que li enviem i ho guarda al fitxer ls.data
void FR_file_ls(int fd, char *path)
{
    pid_t pid;

    // Carreguem la info de la comanda que volem fer
    char *args[] = {"ls", path, NULL};

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

        // Fem l'execucio
        if (execvp(args[0], args) == -1)
        {
            FR_fork_clear();
        }
        break;
    default:
        wait(NULL);
        break;
    }
}

// Retorna la mida d'un fitxer local
int FR_file_length(char *file)
{
    int fd, rc;
    struct stat file_stat;

    // Provem a obrir-lo
    fd = open(file, O_RDONLY);

    // Si no el tenim, informem i sortim
    if (fd < 0)
    {
        return -1;
    }

    rc = fstat(fd, &file_stat);
    if (rc != 0 || S_ISREG(file_stat.st_mode) == 0)
    {
        return -1;
    }

    close(fd);

    return file_stat.st_size;
}

// Retorna el path preparat per tal d'obrir el fitxer enviat
char *FR_file_getpath(char *file)
{
    int i, j;
    char *path;

    path = malloc(sizeof(char));

    // Introduim la info del directori
    for (i = 0; i < (int)strlen(g_config_fremen.path); i++)
    {
        path[i] = g_config_fremen.path[i];
        path = realloc(path, (i + 2) * sizeof(char));
    }

    // Separem amb la barra
    path[i] = '/';
    path = realloc(path, (i + 2) * sizeof(char));
    i++;

    // Introduim la info del fitxer
    for (j = 0; j < (int)strlen(file); i++, j++)
    {
        path[i] = file[j];
        path = realloc(path, (i + 2) * sizeof(char));
    }
    path[i] = '\0';

    return path;
}

// Handles SIGPIPE per tal de saber quan ens han tallat la connexio del servidor
void FR_server_lost()
{
    // Tanquem el fd del servidor i el posem a -1
    close(g_server_fd);
    g_server_fd = -1;

    // Alliberem la memoria corresponent a la connexio amb el servidor
    free(g_user_name);
    free(g_user_code);

    // Informem que hem sigut desconnectats del servidor
    print(FR_ATR_CON_LOST);

    // Reprogramem una altra vegada el SIGPIPE per si recuperem la connexio
    signal(SIGPIPE, FR_server_lost);
}

// Obte el codi d'una comanda
int FR_com_getcode()
{
    int returnCode;
    char *low_command = EXTRA_lower_string(g_com.command_line[0]);

    if (!strcmp(low_command, "login"))
    {
        returnCode = LOGIN;
    }
    else
    {
        if (!strcmp(low_command, "search"))
        {
            returnCode = SEARCH;
        }
        else
        {
            if (!strcmp(low_command, "send"))
            {
                returnCode = SEND;
            }
            else
            {
                if (!strcmp(low_command, "photo"))
                {
                    returnCode = PHOTO;
                }
                else
                {
                    if (!strcmp(low_command, "logout"))
                    {
                        returnCode = LOGOUT;
                    }
                    else
                    {
                        returnCode = LINUX;
                    }
                }
            }
        }
    }
    free(low_command);
    return returnCode;
}

// Allibera tota la memoria utilitzada per la comanda
void FR_com_free()
{
    int i;
    // Alliberem per si es fa un control C
    if (g_com.num_param == 0)
    {
        free(g_com.command_line[0]);
    }
    // Alliberem la memoria que quedi si es una comanda normal
    else
    {
        for (i = 0; i < g_com.num_param; i++)
        {
            free(g_com.command_line[i]);
        }
    }
    free(g_com.command_line);
}

// Allibera la info del fitxer de configuracio
void FR_config_free()
{
    free(g_config_fremen.ip);
    free(g_config_fremen.path);
}

// Envia un missatge al servidor dient que ens hem desconnectat
void FR_server_disconnect()
{
    char buffer[250];
    sprintf(buffer, "%d*%s", g_user_id, g_user_name);
    Trama send_trama = fillTrama(O_FREMEN, T_DISCONNECT, buffer);
    write(g_server_fd, &send_trama, sizeof(Trama));
    close(g_server_fd);
    free(g_user_name);
    free(g_user_code);
    print(FR_ATR_DISC);
}

// Si ens peta un fork, netejem la memoria del fork i sortim
void FR_fork_clear()
{
    // Alliberem memoria del servidor
    if (g_server_fd >= 0)
    {
        free(g_user_name);
        free(g_user_code);
        close(g_server_fd);
    }

    // alliberem la memoria que queda pendent
    FR_com_free();
    FR_config_free();

    // tanquem els file descriptors
    close(0);
    close(1);
    close(2);

    // Tornem el CTRL+C a la funcio original
    signal(SIGINT, SIG_DFL);

    // Ens despedim i tirem un SIGINT per parar l'execucio
    raise(SIGINT);
}

// Informa que s'ha rebut una trama incorrecte
void FR_trama_unknown()
{
    Trama send_trama = fillTrama(O_ATREIDES, T_UNKNOWN, "ERROR T");
    write(g_server_fd, &send_trama, sizeof(Trama));
}

// Printa una linia de codi per pantalla
void print(char *buffer)
{
    write(1, buffer, strlen(buffer));
}