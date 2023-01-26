#include "Atreides.h"

// Funcio principal. Fa la configuracio inicial i gestiona les connexions al servidor
int main(int argc, char *argv[])
{
    // Declarem les variables locals
    int res;
    int new_client_fd;

    // Presentem el servidor
    print(ATR_MSG_INIT);

    // Guardem lloc pel primer usuari i li fiquem id a -1
    g_list_users = malloc(sizeof(User));

    // Inicialitzem la variable de nombre d'usuaris
    g_num_users = 0;

    // Llegim l'arxiu de configuracio i carreguem la variable config
    print(ATR_MSG_READ_CONF);
    ATR_read_conf(argc, argv);

    // Llegim el fitxer amb tots els usuaris
    ATR_read_usersfile();

    // Redefinim CTRL+C ara que ja hem reservat memoria
    signal(SIGINT, ATR_server_close);

    // Iniciem el servidor
    ATR_server_launch();

    // Inicialitzem variables
    g_list_threads = malloc(sizeof(pthread_t));
    g_list_fd_clients = malloc(sizeof(int));
    g_num_clients = 0;

    // Preparem per crear threads detached
    pthread_attr_init(&detached_attr);
    pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);

    // Esperem per connexions
    print(ATR_MSG_WAIT);
    while (1)
    {
        // Fem un bucle per tal de no seguir si no s'ha fet bé el thread del client
        do
        {
            // Bloquejem el socket i activem la connexio
            new_client_fd = accept(g_fd_server, (struct sockaddr *)NULL, NULL);

            // Creem i utilitzem els threads i mirem que s'hagi fet bé el thread
            res = pthread_create(&g_list_threads[g_num_clients], &detached_attr, ATR_server_runclient, &new_client_fd);
            // res = pthread_create(&g_list_threads[g_num_clients], NULL, ATR_server_runclient, &new_client_fd);
            if (res < 0)
            {
                print(SERR_THREAD_CREAT);
            }
        } while (res < 0);

        // pthread_detach(g_list_threads[g_num_clients]);

        // Mentre registrem el socket, no permetem que ningu faci logout
        ATR_mutex_lock(mtx_logout);

        // Incrementem en 1 el client
        g_num_clients++;

        // Si hi ha més d'un client, fem un realloc de l'array de clients
        if (g_num_clients > 0)
        {
            g_list_fd_clients = realloc(g_list_fd_clients, (g_num_clients + 1) * sizeof(int));
            g_list_threads = realloc(g_list_threads, (g_num_clients + 1) * sizeof(pthread_t));
        }

        // Afegim el file descriptor assignat al client a la llista de file descriptors dels clients
        g_list_fd_clients[g_num_clients - 1] = new_client_fd;

        // Tornem a permetre que la gent faci logout
        ATR_mutex_unlock(mtx_logout);
    }
}

// Thread per a cada client que es vagi connectant, conte el codi principal
static void *ATR_server_runclient(void *arg)
{

    // Creem la variable de l'usuari que emplenarem en el login
    User user;
    Trama rec_trama;

    // Agafem el fd del client en concret
    int fd_client = *((int *)arg);

    // Fem el login amb el client
    user = ATR_client_login(fd_client);

    // Mirem si hi ha hagut algun error fent login
    if (user.id == -1)
    {
        return NULL;
    }

    // Fem un bucle per tal de mantenir connectat al client i respondre a les seves peticions
    do
    {
        // Fem que es pugui fer un cancel
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        // Llegim la instrucció
        read(fd_client, &rec_trama, sizeof(Trama));

        // Fem que el thread no es pugui cancelar fins que acabi les operacions
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        // Mirem quina es la instrucció que ens ha enviat
        switch (rec_trama.tipus)
        {
        case T_CONNECT:
            break;
        case T_SEARCH:
            ATR_client_search(rec_trama, fd_client);
            print(ATR_MSG_WAIT);
            break;

        case T_SEND:
            ATR_client_send(user, rec_trama, fd_client);
            print(ATR_MSG_WAIT);
            break;

        case T_PHOTO:
            ATR_client_photo(user, rec_trama, fd_client);
            print(ATR_MSG_WAIT);
            break;

        case T_DISCONNECT:
            // Do nothing
            break;

        default:
            ATR_trama_unknown(fd_client);
            print(SERR_UNKNOWN_RESP);
            print(ATR_MSG_WAIT);
            break;
        }
    } while (rec_trama.tipus != T_DISCONNECT && EXTRA_equals_ignore_case(rec_trama.origen, O_FREMEN));

    ATR_client_logout(user, fd_client);

    // Tanquem el thread
    return NULL;
}

// Funcio encarregada de fer el log out d'un client
void ATR_client_logout(User user, int fd_client)
{
    int i, j;
    char buffer[100];
    int *copy_array_fd = malloc(sizeof(int));
    pthread_t *copy_array_threads = malloc(sizeof(pthread_t));

    // Informem que hem rebut una sol·licitud de desconnexio del usuari
    sprintf(buffer, ATR_LOUT_REC, user.name, user.id);
    print(buffer);

    // Bloquejem el recurs de fer logout
    ATR_mutex_lock(mtx_logout);

    // Retoquem la llista de fd dels clients per tal d'eliminar aquest
    for (i = 0, j = 0; i < g_num_clients; i++)
    {
        if (g_list_fd_clients[i] != fd_client)
        {
            copy_array_fd[j] = g_list_fd_clients[i];
            copy_array_fd = realloc(copy_array_fd, (j + 2) * sizeof(int));

            copy_array_threads[j] = g_list_threads[i];
            copy_array_threads = realloc(copy_array_threads, (j + 2) * sizeof(pthread_t));
            j++;
        }
    }

    // Tanquem el fd del client que estavem utilitzant
    close(fd_client);

    // Baixem el nombre de clients connectats
    g_num_clients--;

    // Alliberem l'anterior array i l'actualitzem
    free(g_list_fd_clients);
    g_list_fd_clients = copy_array_fd;

    free(g_list_threads);
    g_list_threads = copy_array_threads;

    // Desbloquejem el recurs de fer logout
    ATR_mutex_unlock(mtx_logout);

    // Informem que l'hem desconnectat be
    print(ATR_LOUT_DONE);
    print(ATR_MSG_WAIT);

    // Tanquem el thread
    pthread_exit(NULL);
}

// Funcio encarregadad de fer la comanda photo
void ATR_client_photo(User user, Trama rec_trama, int fd_client)
{
    // Declaracio de variables locals
    Trama send_trama;
    int id_image, fd_image, fd_md5sum, size, i;
    char *path_image, *name_image, *path_md5sum, *md5sum, buffer[100];

    // Llegim el contingut de la trama
    id_image = atoi(rec_trama.data);

    // Informem que hem rebut la peticio
    sprintf(buffer, ATR_PHOTO_REC, id_image, user.name, user.id);
    print(buffer);

    // Recorrem la llista buscant
    for (i = 0; i < g_num_users && id_image != g_list_users[i].id; i++)
    {
    }

    // Comprobem que l'id existis
    if (i == g_num_users)
    {
        // Print informatiu pel servidor
        print(SERR_UNEXISTENT_ID);

        // Informem que no s'ha pogut trobar el fitxer a l'usuari
        send_trama = fillTrama(O_ATREIDES, T_PHOTO_E, SERR_UNEXISTENT_ID);
        write(fd_client, &send_trama, sizeof(Trama));
        return;
    }

    // Mirem el nom de la imatge
    sprintf(buffer, "%d.jpg", id_image);
    name_image = EXTRA_copy_string(buffer);

    // Aconseguim el path que hauria de tenir la imatge
    path_image = ATR_file_getpath(name_image);

    // Bloquejem el recurs de les imatges
    ATR_mutex_lock(mtx_images);

    // Provem a obrir el path de la imatge
    fd_image = open(path_image, O_RDONLY);
    if (fd_image < 0)
    {
        ATR_mutex_unlock(mtx_images);

        // Informem que no hi ha foto registrada
        print(ATR_PHOTO_NR);

        // Informem que no s'ha pogut trobar el fitxer a l'usuari
        send_trama = fillTrama(O_ATREIDES, T_PHOTO_F, D_PHOTO_NOT_FOUND);
        write(fd_client, &send_trama, sizeof(Trama));

        // Informem que s'ha enviat resposta
        print(ATR_PHOTO_ANSWERED);

        // Fem els frees corresponents
        free(path_image);
        free(name_image);
        return;
    }

    // Mirem la mida del fitxer
    size = ATR_file_length(path_image);

    // Guardem el path on trobarem md5sum
    path_md5sum = ATR_file_getpath(MD5SUM_FILE);

    // Bloquejem el recurs del md5sum
    ATR_mutex_lock(mtx_md5sum);

    // Escribim al fitxer md5sum la informacio del md5sum
    fd_md5sum = open(path_md5sum, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd_md5sum < 0)
    {
        ATR_mutex_unlock(mtx_images);
        ATR_mutex_unlock(mtx_md5sum);
        free(path_md5sum);
        free(name_image);
        print(ERROR_FILE_O);
        return;
    }
    ATR_file_md5sum(fd_md5sum, path_image);

    // Tanquem el fitxer en format d'escriptura
    close(fd_md5sum);

    // Llegim la info del md5sum
    fd_md5sum = open(path_md5sum, O_RDONLY);
    if (fd_md5sum < 0)
    {
        ATR_mutex_unlock(mtx_images);
        ATR_mutex_unlock(mtx_md5sum);
        free(path_image);
        free(path_md5sum);
        free(name_image);
        print(ERROR_FILE_O);
        return;
    }

    // Llegim la primera linia i agafem el md5sum del fitxer
    md5sum = EXTRA_read_line(fd_md5sum, ' ');

    // Tanquem el fd
    close(fd_md5sum);

    // Desbloquejem el recurs del md5sum
    ATR_mutex_unlock(mtx_md5sum);

    // Informem al client amb tot el que necessitara sobre la imatge
    sprintf(buffer, "%s*%d*%s", name_image, size, md5sum);
    send_trama = fillTrama(O_ATREIDES, T_PHOTO_F, buffer);
    write(fd_client, &send_trama, sizeof(Trama));

    // Alliberem la informacio que ja no necessitem
    free(path_md5sum);
    free(md5sum);

    // Informem del que enviarem
    sprintf(buffer, ATR_PHOTO_SEND, name_image);
    print(buffer);

    // Alliberem el nom de la imatge
    free(name_image);

    // Inicialitzem la trama que volem enviar
    send_trama = fillTramaNoData(O_FREMEN, T_PHOTO_D);

    // Enviem la info del fitxer per trames fins que acabem el fitxer
    while (size > IMAGE_DATA_SIZE)
    {
        // Per cada trama, emplenem les dades que hem decidit
        read(fd_image, &send_trama.data, IMAGE_DATA_SIZE * sizeof(char));

        // Emplenem la trama que queda amb '\0'
        for (i = IMAGE_DATA_SIZE; i < 240; i++)
        {
            send_trama.data[i] = '\0';
        }

        // Enviem la trama
        write(fd_client, &send_trama, sizeof(Trama));

        // Reconfigurem el valor de la mida que queda per enviar
        size -= IMAGE_DATA_SIZE;
    }

    // Si queda info a enviar, enviem
    if (size > 0)
    {
        // Per cada trama, emplenem les dades que poguem
        read(fd_image, &send_trama.data, size * sizeof(char));

        // Emplenem la trama que queda amb '\0'
        for (i = size; i < 240; i++)
        {
            send_trama.data[i] = '\0';
        }

        // Enviem la trama
        write(fd_client, &send_trama, sizeof(Trama));
    }

    // Tanquem el fitxer i alliberem la memoria del path
    close(fd_image);
    free(path_image);

    // Desbloquejem el recurs de les imatges
    ATR_mutex_unlock(mtx_images);

    // Rebem la resposta del thread
    read(fd_client, &rec_trama, sizeof(Trama));

    // Comprobem que sigui correcte
    if (rec_trama.tipus == T_PHOTO_E)
    {
        print(SERR_PHOTO_CORRUPTED);
        return;
    }

    if (rec_trama.tipus != T_PHOTO_OK)
    {
        // Informem al client que s'ha enviat una trama incorrecte
        ATR_trama_unknown(fd_client);

        // Printem per pantalla que hem rebut una trama incorrecte
        print(SERR_UNKNOWN_RESP);
        return;
    }

    // Informem que la foto s'ha enviat correctament
    print(ATR_PHOTO_ANSWERED);
}

// Funcio encarregada de fer el send
void ATR_client_send(User user, Trama rec_trama, int fd_client)
{
    Trama send_trama;
    int fd_image, fd_md5sum, size, i, j;
    char *file_name, *atr_file_name, *md5sum, *aux, *path_image, *path_md5sum;
    char buffer[240];

    // Parsejem el nom de la imatge enviada
    file_name = malloc(sizeof(char));
    for (i = 0, j = 0; rec_trama.data[i] != '*'; i++, j++)
    {
        file_name[j] = rec_trama.data[i];
        file_name = realloc(file_name, (j + 2) * sizeof(char));

        // Mirem que no se'ns hagi enviat un nom massa llarg
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

    // Informem que hem rebut el send
    sprintf(buffer, ATR_SEND_REC, file_name, user.name, user.id);
    print(buffer);

    // Fem el free del nom del fitxer, ja que no el tornarem a utilitzar
    free(file_name);

    // Guardem el path del lloc on haura d'anar la nostra imatge
    sprintf(buffer, "%d.jpg", user.id);
    atr_file_name = EXTRA_substring(buffer, 0, strlen(buffer) - 1);
    path_image = ATR_file_getpath(atr_file_name);

    // Bloquejem el recurs d'accés a les imatges
    ATR_mutex_lock(mtx_images);

    // Esborrem el contingut si hi ha algun creat i el tornem a crear
    fd_image = open(path_image, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd_image < 0)
    {
        ATR_mutex_unlock(mtx_images);
        free(md5sum);
        free(atr_file_name);
        free(path_image);
        print(ERROR_FILE_O);
        return;
    }

    // Rebem totes les trames alhora que emplenem el fitxer de la imatge amb la info que rebem
    while (size > IMAGE_DATA_SIZE)
    {
        // Llegim la trama
        read(fd_client, &rec_trama, sizeof(Trama));

        // Comprobem que sigui del tipus correcte
        if (rec_trama.tipus != T_SEND_D)
        {
            // Informem al client
            ATR_trama_unknown(fd_client);

            // Printem per pantalla
            print(SERR_UNKNOWN_RESP);

            // Fem les accions restants abans de sortir
            ATR_mutex_unlock(mtx_images);
            close(fd_image);
            free(md5sum);
            free(atr_file_name);
            free(path_image);
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
        read(fd_client, &rec_trama, sizeof(Trama));

        // Comprobem que sigui del tipus correcte
        if (rec_trama.tipus != T_SEND_D)
        {
            // Informem al client
            ATR_trama_unknown(fd_client);

            // Printem per pantalla
            print(SERR_UNKNOWN_RESP);

            // Fem les accions restants abans de sortir
            ATR_mutex_unlock(mtx_images);
            close(fd_image);
            free(md5sum);
            free(atr_file_name);
            free(path_image);
            return;
        }

        // Escribim la informacio
        write(fd_image, &rec_trama.data, size * sizeof(char));
    }

    // Tanquem el fd de la imatge
    close(fd_image);

    // Desbloquejem el recurs d'accés a les imatges
    ATR_mutex_unlock(mtx_images);

    // Guardem el path on trobarem md5sum
    path_md5sum = ATR_file_getpath(MD5SUM_FILE);

    // Bloquejem el recurs del fitxer de md5sum
    ATR_mutex_lock(mtx_md5sum);

    // Escribim al fitxer md5sum la informacio del md5sum
    fd_md5sum = open(path_md5sum, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd_md5sum < 0)
    {
        ATR_mutex_unlock(mtx_md5sum);
        free(path_md5sum);
        free(atr_file_name);
        print(ERROR_FILE_O);
        return;
    }
    ATR_file_md5sum(fd_md5sum, path_image);

    // Tanquem el fitxer en format d'escriptura
    close(fd_md5sum);

    // Llegim la info del md5sum
    fd_md5sum = open(path_md5sum, O_RDONLY);
    if (fd_md5sum < 0)
    {
        ATR_mutex_unlock(mtx_md5sum);
        free(md5sum);
        free(atr_file_name);
        free(path_image);
        free(path_md5sum);
        print(ERROR_FILE_O);
        return;
    }

    // Llegim la primera linia i agafem el md5sum del fitxer
    aux = EXTRA_read_line(fd_md5sum, ' ');

    // Tanquem el fd
    close(fd_md5sum);

    // Desbloquejem el recurs del fitxer md5sum
    ATR_mutex_unlock(mtx_md5sum);

    // Mirem si el md5sum actual es el mateix que el que hauriem de tenir
    if (strcmp(md5sum, aux) != 0)
    {
        // Enviem una trama informant que ha anat malament
        send_trama = fillTrama(O_ATREIDES, T_SEND_E, "IMAGE KO");
        write(fd_client, &send_trama, sizeof(Trama));

        // Informem que les dades han estat contaminades
        print(ATR_SEND_CORRUPTED);

        // Alliberem la memoria abans de sortir
        free(md5sum);
        free(atr_file_name);
        free(path_image);
        free(path_md5sum);
        free(aux);
        return;
    }

    // Informem que l'hem guardat
    sprintf(buffer, ATR_SEND_DONE, atr_file_name);
    print(buffer);

    // Enviem a Fremen informant que tot ha anat be
    send_trama = fillTrama(O_ATREIDES, T_SEND_OK, "IMAGE OK");
    write(fd_client, &send_trama, sizeof(Trama));

    // Fem els frees que ens hagin quedat pendents
    free(md5sum);
    free(path_image);
    free(path_md5sum);
    free(aux);
    free(atr_file_name);
}

// Funcio encarregada de fer el search
void ATR_client_search(Trama rec_trama, int fd_client)
{
    int i, j, k, num_persons, num_trames, search_id;
    char buffer[240], *string, *aux_id, *search_name, *search_code, *result;
    User *persons;
    Trama send_trama;

    // Agafem el nom de la trama
    search_name = malloc(sizeof(char));
    for (i = 0; rec_trama.data[i] != '*'; i++)
    {
        search_name[i] = rec_trama.data[i];
        search_name = realloc(search_name, (i + 2) * sizeof(char));

        // Mirem que no estigui llegint més del compte
        if (i > 22)
        {
            send_trama = fillTrama(O_ATREIDES, T_SEARCH_E, "0");
            write(fd_client, &send_trama, sizeof(Trama));
            print(SERR_DATA_FORMAT);
            free(search_name);
            return;
        }
    }
    search_name[i] = '\0';
    i++;

    // Agafem la id de la trama
    aux_id = malloc(sizeof(char));
    for (j = 0; rec_trama.data[i] != '*'; i++, j++)
    {
        aux_id[j] = rec_trama.data[i];
        aux_id = realloc(aux_id, (j + 2) * sizeof(char));

        // Mirem que no estigui llegint més del compte
        if (j > 8)
        {
            send_trama = fillTrama(O_ATREIDES, T_SEARCH_E, "0");
            write(fd_client, &send_trama, sizeof(Trama));
            print(SERR_DATA_FORMAT);
            free(search_name);
            free(aux_id);
            return;
        }
    }
    aux_id[j] = '\0';
    i++;

    // Agafem el codi de la trama
    search_code = malloc(sizeof(char));
    for (j = 0; rec_trama.data[i] != '\0'; i++, j++)
    {
        search_code[j] = rec_trama.data[i];
        search_code = realloc(search_code, (j + 2) * sizeof(char));

        // Mirem que no estigui llegint més del compte
        if (j > 5)
        {
            send_trama = fillTrama(O_ATREIDES, T_SEARCH_E, "0");
            write(fd_client, &send_trama, sizeof(Trama));
            print(SERR_DATA_FORMAT);
            free(search_name);
            free(aux_id);
            free(search_code);
            return;
        }
    }
    search_code[j] = '\0';

    // Mirem que el codi postal estigui composat per 5 digits (obligatori)
    if ((int)strlen(search_code) != 5)
    {
        send_trama = fillTrama(O_ATREIDES, T_SEARCH_E, "0");
        write(fd_client, &send_trama, sizeof(Trama));
        print(SERR_DATA_FORMAT);
        free(search_name);
        free(aux_id);
        free(search_code);
        return;
    }

    // Informem que hem rebut un searh
    search_id = atoi(aux_id);
    sprintf(buffer, ATR_SRCH_REC, search_code, search_name, search_id);
    print(buffer);
    free(aux_id);

    // Bloquejem el recurs de la user list
    ATR_mutex_lock(mtx_user_list);

    // Fem la cerca
    persons = malloc(sizeof(User));
    for (i = 0, num_persons = 0; i < g_num_users; i++)
    {
        if (EXTRA_equals_ignore_case(g_list_users[i].code, search_code))
        {
            if (g_list_users[i].id != search_id)
            {
                persons[num_persons] = g_list_users[i];
                persons = realloc(persons, (num_persons + 2) * sizeof(User));
                num_persons++;
            }
        }
    }

    // Desbloquejem el recurs de la user list
    ATR_mutex_unlock(mtx_user_list);

    // Informem que hem fet la cerca
    print(ATR_SRCH_DONE);
    sprintf(buffer, ATR_SRCH_RESP_FORMAT, num_persons, search_code);
    print(buffer);

    // Formatejem el resultat de la cerca i ho fiquem tot dins un string
    string = malloc(sizeof(char));
    for (i = 0, k = 0; i < num_persons; i++)
    {
        // Copiem la id
        sprintf(buffer, "%d", persons[i].id);
        for (j = 0; j < (int)strlen(buffer); j++, k++)
        {
            string[k] = buffer[j];
            string = realloc(string, (k + 2) * sizeof(char));
        }

        // Posem un espai per formatejar
        string[k] = ' ';
        string = realloc(string, (k + 2) * sizeof(char));
        k++;

        // Copiem el nom
        for (j = 0; j < (int)strlen(persons[i].name); j++, k++)
        {
            string[k] = persons[i].name[j];
            string = realloc(string, (k + 2) * sizeof(char));
        }

        // Posem un salt de linia per formatejar
        string[k] = '\n';
        string = realloc(string, (k + 2) * sizeof(char));
        k++;
    }

    // Tanquem el string
    string[k] = '\0';

    // Informem del resultat de la cerca
    print(string);

    // Formatejem el string per poder enviar la resposta
    for (i = 0; i < (int)strlen(string); i++)
    {
        if (string[i] == ' ' || (string[i] == '\n' && string[i + 1] != '\0'))
        {
            string[i] = '*';
        }
    }

    // Mirem quantes trames haurem d'enviar, ho he fet amb 220 en lloc de 240 per tenir marge
    num_trames = ((int)strlen(string) / 220) + 1;

    // Insertem al string de resultat el nombre de trames que enviarem
    result = malloc(sizeof(char));
    sprintf(buffer, "%d", num_trames);
    for (i = 0; i < (int)strlen(buffer); i++)
    {
        result[i] = buffer[i];
        result = realloc(result, (i + 2) * sizeof(char));
    }

    // Insertem l'asterisc * per tal de respectar el format de la trama
    result[i] = '*';
    result = realloc(result, (i + 2) * sizeof(char));
    i++;

    // Insertim al string de resultat la resta de la resposta
    for (j = 0; j < (int)strlen(string); j++, i++)
    {
        result[i] = string[j];
        result = realloc(result, (i + 2) * sizeof(char));
    }
    result[i] = '\0';
    free(string);

    // Abans de començar mirem a quina posicio es troba el primer asterisc de la trama per descartar el "quants" i ens el saltem
    j = (int)strlen(buffer) + 1;

    // Enviem totes les trames que facin falta
    for (i = 0; i < num_trames; i++)
    {
        // Mirem si estem a la ultima trama a enviar
        if ((i + 1) == num_trames)
        {
            // Enviem la trama tal qual
            send_trama = fillTrama(O_ATREIDES, T_SEARCH_OK, result);
            write(fd_client, &send_trama, sizeof(Trama));
            break;
        }

        // Si encara hem d'enviar mes trames despres, fem tot el proces
        for (k = 0; j < (int)strlen(result); j++)
        {
            if (result[j] == '*')
            {
                k++;
                // Si ja hem introduit un usuari sencer, mirem si ens hi cap un altre (30 espais)
                if (k % 2 == 0)
                {
                    // Si no hem acabat, mirem si queda espai en aquesta trama
                    if ((240 - j) < 30)
                    {
                        // Hem utilitzat 30 espais perquè creiem que no serà mai més gran un nom(22) + id(8)
                        string = EXTRA_substring(result, 0, j - 1);
                        send_trama = fillTrama(O_ATREIDES, T_SEARCH_OK, string);
                        write(fd_client, &send_trama, sizeof(Trama));

                        // Alliberem la memoria que acabem d'utilitzar per poder reassignar la variable
                        free(string);

                        // Li traiem a result la part de string que hem enviat utilitzant una variable auxiliar per tal de no perdre memoria dinamica
                        string = EXTRA_substring(result, j + 1, strlen(result) - 1);
                        free(result);
                        result = EXTRA_copy_string(string);
                        free(string);

                        // Sortim del bucle perque ja hem enviat la trama corresponent
                        break;
                    }
                }
            }

            // Comprovem que la j no es passi de la llargada maxima per prevenir errors
            if (j > 240 && j != 300)
            {
                print(SERR_SEARCH_RESP);
                send_trama = fillTrama(O_ATREIDES, T_SEARCH_E, "0");
                write(fd_client, &send_trama, sizeof(Trama));
                free(search_name);
                free(aux_id);
                free(search_code);
                free(result);
            }
        }
        // Una vegada passem el primer bucle, fiquem sempre la j a 0
        j = 0;
    }

    // Informem que ja hem enviat la resposta
    print(ATR_SRCH_ANSWERED);

    // Fem els frees necessaris
    free(search_name);
    free(search_code);
    free(persons);
    free(result);
}

// Funció encarregada de fer el login, si la trama no es de FREMEN i no es del tipus CONNECT, retorna un usuari amb id -1
User ATR_client_login(int fd_client)
{
    Trama rec_trama, send_trama;
    User user;
    int i, j;
    char *name, *code, buffer[100], *line;

    // Llegim la trama enviada per l'usuari
    read(fd_client, &rec_trama, sizeof(Trama));

    // Comprovem que sigui de tipus LogIn i enviada per FREMEN
    if (!(rec_trama.tipus == T_CONNECT && EXTRA_equals_ignore_case(rec_trama.origen, O_FREMEN)))
    {
        // Informem al que s'ha intentat connectar que la seva trama contenia errors
        send_trama = fillTrama(O_ATREIDES, T_CONNECT_E, "ERROR");
        write(fd_client, &send_trama, sizeof(Trama));

        // Printem per pantalla que hi ha hagut un error al connectar
        print(SERR_LOGIN);

        // Si no es correcte, fiquem la id de l'usuari a -1 per informar
        user.id = -1;

        // Retornem l'usuari amb id -1, sortint de la funcio server_login()
        return user;
    }

    // Una vegada tenim una trama de login correcte, emplenem l'usuari amb les dades enviades
    name = malloc(sizeof(char));
    for (i = 0, j = 0; rec_trama.data[i] != '*'; i++, j++)
    {
        name[j] = rec_trama.data[i];
        name = realloc(name, (j + 2) * sizeof(char));

        // Fiquem un filtre en cas que s'hagi enviat malament la informacio
        if (j > 30)
        {
            // Informem a FREMEN que s'ha connectat malament
            send_trama = fillTrama(O_ATREIDES, T_CONNECT_E, "ERROR");
            write(fd_client, &send_trama, sizeof(Trama));

            // Printem per pantalla que hi ha hagut un error
            print(SERR_LOGIN);

            // Alliberem la variable name
            free(name);

            // Posem user amb id -1
            user.id = -1;

            // Retornem l'usuari
            return user;
        }
    }

    // Tanquem el string de name
    name[j] = '\0';

    // Passem a registrar code
    i++;
    code = malloc(sizeof(char));
    for (j = 0; rec_trama.data[i] != '\0'; i++, j++)
    {
        code[j] = rec_trama.data[i];
        code = realloc(code, (j + 2) * sizeof(char));

        // Fiquem un filtre en cas que s'hagi enviat malament la informacio
        if (j > 5)
        {
            // Informem a FREMEN que s'ha connectat malament
            send_trama = fillTrama(O_ATREIDES, T_CONNECT_E, "ERROR");
            write(fd_client, &send_trama, sizeof(Trama));

            // Printem per pantalla que hi ha hagut un error
            print(SERR_LOGIN);

            // Alliberem la variable name
            free(name);
            free(code);

            // Posem user amb id -1
            user.id = -1;

            // Retornem l'usuari
            return user;
        }
    }
    // Tanquem el string de code
    code[j] = '\0';

    // Printem que hem rebut un login amb el nom i codi que ens han enviat
    sprintf(buffer, ATR_LIN_REC, name, code);
    print(buffer);

    // Ens assegurem que som els unics utilitzant la variable global de user list
    ATR_mutex_lock(mtx_user_list);

    //  Ara mirem si aquest usuari ja existeix (mirant el nom)
    for (i = 0; i < g_num_users; i++)
    {
        // Si passa aquest if, l'usuari ja estava previament registrat
        if (EXTRA_equals_ignore_case(name, g_list_users[i].name))
        {
            // Guardem l'usuari
            user = g_list_users[i];

            // Si no tenen el mateix codi postal, l'actualitzem a la llista i al fitxer
            if (!EXTRA_equals_ignore_case(code, g_list_users[i].code))
            {
                // Guardem el format actual de la linia per poder-lo eliminar mes tard
                line = ATR_format_user(g_list_users[i]);

                // Primer alliberem la memoria del codi actual
                free(g_list_users[i].code);

                // Despres copiem la referencia del nostre codi al usuari
                g_list_users[i].code = code;

                // Guardem l'usuari
                user = g_list_users[i];

                // Actualitzem l'usuari a la base de dades
                ATR_file_modify_user(user, line);
            }
            // Si tenen el mateix codi postal, alliberem la memoria del codi enviat
            else
            {
                free(code);
            }

            // Alliberem la memoria dinamica del nom
            free(name);

            // Sortim del bucle
            break;
        }
    }

    // Mirem si l'usuari no existeix i, per tant, l'hem de crear
    if (i == g_num_users)
    {
        //  Augmentem l'espai de la llista d'usuaris
        if (g_num_users > 0)
        {
            g_list_users = realloc(g_list_users, (g_num_users + 1) * sizeof(User));
        }

        // Afegim l'usuari a la llista d'usuaris
        g_list_users[g_num_users].id = ATR_format_random_id();
        g_list_users[g_num_users].name = name;
        g_list_users[g_num_users].code = code;

        // Registrem l'usuari al fitxer d'usuaris
        ATR_file_register_user(g_list_users[g_num_users]);

        // Ens guardem l'usuari
        user = g_list_users[g_num_users];

        // Augmentem el nombre d'usuaris
        g_num_users++;
    }

    // Aqui ja no modifiquem res relacionat amb la llista d'usuaris global
    ATR_mutex_unlock(mtx_user_list);

    // Printem que li hem assignat una id
    sprintf(buffer, ATR_LIN_ID_ASIG, user.id);
    print(buffer);

    // Enviem el missatge a l'usuari que ha fet login amb exit i li retornem la id
    sprintf(buffer, "%d", user.id);
    send_trama = fillTrama(O_ATREIDES, T_CONNECT_OK, buffer);
    write(fd_client, &send_trama, sizeof(Trama));

    // Printem que ja li hem enviat una resposta
    print(ATR_LIN_ANSWERED);

    // Retornem l'usuari
    return user;
}

// Informa que s'ha rebut una trama incorrecte
void ATR_trama_unknown(int fd_client)
{
    Trama send_trama = fillTrama(O_ATREIDES, T_UNKNOWN, "ERROR T");
    write(fd_client, &send_trama, sizeof(Trama));
}

// Executa md5sum del path que li enviem i ho guarda al fitxer md5sum
void ATR_file_md5sum(int fd, char *path)
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

// Actualitza un usuari del fitxer de la llista d'usuaris registrats
void ATR_file_modify_user(User user, char *actual)
{
    // Variables que necessitarem
    int fd;
    char *line, *first, *second;
    off_t offset;

    // Guardem el path
    line = ATR_file_getpath(USERS_DATA_FILE);

    // Obrim el fitxer d'usuaris en format escriptura
    fd = open(line, O_RDWR);
    if (fd < 0)
    {
        free(line);
        print(SERR_USERS_FILE);
        return;
    }

    // Alliberem la memoria que hem utilitzat pel path
    free(line);

    // Agafem els 5 primers del que ens han enviat i l'alliberem
    first = EXTRA_substring(actual, 0, 5);
    free(actual);

    // Guardem la posicio inicial del offset
    offset = lseek(fd, 0, SEEK_CUR);

    // Anem llegint linies fins trobar la linia a modificar
    line = EXTRA_read_line(fd, '\n');
    second = EXTRA_substring(line, 0, 5);
    while (!EXTRA_equals_ignore_case(first, second))
    {
        // Alliberem la memoria dels dos
        free(line);
        free(second);

        // Guardem la posicio on hem acabat abans de llegir la seguent linia
        offset = lseek(fd, 0, SEEK_CUR);

        // Reassignem el valor dels dos
        line = EXTRA_read_line(fd, '\n');
        second = EXTRA_substring(line, 0, 5);

        // Mirem que no haguem acabat ja el fitxer, informatiu pero no hauria de passar mai
        if (EXTRA_equals_ignore_case(line, "\0"))
        {
            free(line);
            free(first);
            free(second);
            close(fd);
            print(SERR_MODIFY_USER);
            return;
        }
    }

    // Alliberem la memoria de first i second perque ja no la utilitzarem
    free(first);
    free(second);

    // Quan ja hem arribat a la actual, reposicionem el punter una linia enrere
    lseek(fd, offset, SEEK_SET);

    // Alliberem memoria de la linia
    free(line);

    // La assignem al nou valor que guardarem
    line = ATR_format_user(user);

    // Escribim la linia actual
    write(fd, line, strlen(line));

    // Alliberem tambe la linia que hem utilitzat
    free(line);

    // Tanquem el fd del fitxer
    close(fd);
}

// Afegeix un usuari al fitxer on guardem els usuaris registrats
void ATR_file_register_user(User user)
{
    // Variables que necessitarem
    int fd;
    char *line;

    // Preparem el path
    line = ATR_file_getpath(USERS_DATA_FILE);

    // Obrim el fitxer d'usuaris en format escriptura
    fd = open(line, O_WRONLY | O_APPEND);
    if (fd < 0)
    {
        free(line);
        print(SERR_USERS_FILE);
        return;
    }

    // Alliberem la memoria de la linia del path
    free(line);

    // Formatejem la info del user per tal de poderla guardar al fitxer d'info dels users
    line = ATR_format_user(user);

    // Escribim la linia al fitxer
    write(fd, line, strlen(line));

    // Tanquem el fd del fitxer
    close(fd);

    // Alliberem la memoria de la linia
    free(line);
}

// Retorna una linia amb la info preparada per ser guardada al fitxer d'usuaris
char *ATR_format_user(User user)
{
    // Variables que necessitem
    char *line, buffer[5];
    int i, j;

    // Formatejem la id per tal de convertirla a string
    sprintf(buffer, "%d", user.id);

    // Preparem la linia amb l'espai ja per guardar la id
    line = malloc(sizeof(char));

    // Afegim el contingut de la id a la linia
    for (i = 0; i < (int)strlen(buffer); i++)
    {
        line[i] = buffer[i];
        line = realloc(line, (i + 2) * sizeof(char));
    }

    // Separem la info
    line[i] = '#';
    line = realloc(line, (i + 2) * sizeof(char));
    i++;

    // Afegim el nom a la linia
    for (j = 0; j < (int)strlen(user.name); j++, i++)
    {
        line[i] = user.name[j];
        line = realloc(line, (i + 2) * sizeof(char));
    }

    // Separem la info
    line[i] = '#';
    line = realloc(line, (i + 2) * sizeof(char));
    i++;

    // Afegim el codi postal a la linia
    for (j = 0; j < (int)strlen(user.code); j++, i++)
    {
        line[i] = user.code[j];
        line = realloc(line, (i + 2) * sizeof(char));
    }

    // Finalitzem la linia amb un salt de linia
    line[i] = '\n';
    line = realloc(line, (i + 2) * sizeof(char));
    i++;

    // Tanquem la linia amb un \0
    line[i] = '\0';

    // Retornem la linia
    return line;
}

// Retorna una id que no estigui registrada fins al moment
int ATR_format_random_id()
{
    int id, i;

    // Posem la seed de random en aleatoria per a que no ens doni sempre el mateix
    srand(time(NULL));
    do
    {
        // Generem un nombre entre 0 i 899 i li sumem 100, pel que el rang esta [100, 99999]
        id = (rand() % 99900) + 100;

        // Si hi ha altres usuaris, mirem
        for (i = 0; i < g_num_users; i++)
        {
            // Mirem si ja existeix la id
            if (id == g_list_users[i].id)
            {
                break;
            }
        }

    } while (i < g_num_users);

    return id;
}

// Llegeix l'arxiu de Fremen on estan enregistrats tots els usuaris
void ATR_read_usersfile()
{
    int fd, i, j, pos_user = -1;
    char *line, *aux;

    // Preparem el path
    line = ATR_file_getpath(USERS_DATA_FILE);

    // Mirem si el document existeix, si existeix, emplenem config
    fd = open(line, O_RDONLY | O_CREAT, 0644);
    if (fd == -1)
    {
        free(line);
        print(SERR_USERS_FILE);
        return;
    }

    // Alliberem la memoria de la linia del path
    free(line);

    // Llegim la primera linia del fitxer
    line = EXTRA_read_line(fd, '\n');

    // Bucle per tal de llegir totes les linies del fitxer que queden
    while (!EXTRA_equals_ignore_case(line, "\0"))
    {
        // Augmentem posuser perque entra estant a -1
        pos_user++;

        // Llegim ID
        aux = malloc(sizeof(char));
        for (i = 0, j = 0; line[i] != '#'; i++, j++)
        {
            aux[j] = line[i];
            aux = realloc(aux, (j + 2) * sizeof(char));
        }

        aux[j] = '\0';
        g_list_users[pos_user].id = atoi(aux);
        free(aux);

        // Llegim NOM
        i++;
        g_list_users[pos_user].name = malloc(sizeof(char));
        for (j = 0; line[i] != '#'; i++, j++)
        {
            g_list_users[pos_user].name[j] = line[i];
            g_list_users[pos_user].name = realloc(g_list_users[pos_user].name, (j + 2) * sizeof(char));
        }
        g_list_users[pos_user].name[j] = '\0';

        // Llegim CODI_POSTAL
        i++;
        g_list_users[pos_user].code = malloc(sizeof(char));
        for (j = 0; line[i] != '\0'; i++, j++)
        {
            g_list_users[pos_user].code[j] = line[i];
            g_list_users[pos_user].code = realloc(g_list_users[pos_user].code, (j + 2) * sizeof(char));
        }
        g_list_users[pos_user].code[j] = '\0';

        // Alliberem la memoria de la linia
        free(line);

        // Llegim la seguent linia
        line = EXTRA_read_line(fd, '\n');

        // Si tot esta ok, fem un realloc a user_list per guardar un altre user a la propera iteracio
        if (!EXTRA_equals_ignore_case(line, "\0"))
        {
            g_list_users = realloc(g_list_users, (pos_user + 2) * sizeof(User));
        }
    }

    // Guardem el nombre d'usuaris que s'han afegit a la llista
    g_num_users = pos_user + 1;

    // Alliberem la memoria de la ultima linia llegida
    free(line);

    // Tanquem el fd
    close(fd);
}

// Retorna la mida d'un fitxer local
int ATR_file_length(char *file)
{
    int fd, rc;
    struct stat file_stat;

    // Provem a obrir-lo
    fd = open(file, O_RDONLY);

    // Si no el tenim, informem i sortim
    if (fd < 0)
    {
        print(ERROR_FILE_O);
        return -1;
    }

    rc = fstat(fd, &file_stat);
    if (rc != 0 || S_ISREG(file_stat.st_mode) == 0)
    {
        print(ERROR_FSTAT);
        return -1;
    }

    close(fd);

    return file_stat.st_size;
}

// Retorna el path preparat per tal d'obrir el fitxer enviat
char *ATR_file_getpath(char *file)
{
    int i, j;
    char *path;

    path = malloc(sizeof(char));

    // Introduim la info del directori
    for (i = 0; i < (int)strlen(g_config.path); i++)
    {
        path[i] = g_config.path[i];
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

// Funció encarregada de llegir el fitxer de configuració
void ATR_read_conf(int argc, char *argv[])
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
        g_config.ip = line;

        line = EXTRA_read_line(fd, '\n');
        g_config.port = atoi(line);
        free(line);

        line = EXTRA_read_line(fd, '\n');
        g_config.path = line;

        close(fd);
    }
}

// Funció encarregada d'engegar el servidor per tal de permetre connexions
int ATR_server_launch()
{
    struct sockaddr_in server;

    // Obrim el fd del servidor i mirem que s'hagi obert bé
    g_fd_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_fd_server < 0)
    {
        print(SERR_SOCKET_CREAT);
        return -1;
    }

    // Reservem l'espai del servidor emplentant tot a 0
    bzero(&server, sizeof(server));

    server.sin_port = htons(g_config.port);
    server.sin_family = AF_INET;
    // server.sin_addr.s_addr = inet_addr(config.ip); Funciona pero de moment farem htonl per anar provant
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Enllaçem i comprovem que es faci bé
    if (bind(g_fd_server, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        print(SERR_BIND);
        return -1;
    }

    // Obrim el socket del servidor i determinem el nombre màxim de connexions a acceptar
    if (listen(g_fd_server, MAX_CONNECTIONS_NUM) < 0)
    {
        print(SERR_LISTEN);
        return -1;
    }

    // Si arribem aqui, tot ha anat bé i retornem 1 (nombre positiu) per informar-ho
    return 1;
}

// Lock the mutex passed as a parameter
void ATR_mutex_lock(pthread_mutex_t mutex)
{
    int res;
    res = pthread_mutex_lock(&mutex);
    if (res != 0)
    {
        print(SERR_MUT_LOCK);
    }
}

// Unlock the mutex passed as a parameter
void ATR_mutex_unlock(pthread_mutex_t mutex)
{
    int res;
    res = pthread_mutex_unlock(&mutex);
    if (res != 0)
    {
        print(SERR_MUT_UNLOCK);
    }
}

// Funcio que lliguiem amb el signal SIGINT per tal de tancar el servidor
void ATR_server_close()
{
    // Fen un parell d'espais per a que amb el valgrind es vegi tot bé
    print("\n\n");

    // Tanquem els threads que quedin pendents
    ATR_cancel_threads();

    // alliberem la memoria que queda pendent
    ATR_free_configfile();
    ATR_free_userlist();

    // tanquem els file descriptors relacionats amb sockets
    ATR_close_socketsfd();

    // Fem els detach de tots els threads que s'han creat
    pthread_attr_destroy(&detached_attr);

    // Destruim el mutex
    pthread_mutex_destroy(&mtx_print);
    pthread_mutex_destroy(&mtx_user_list);
    pthread_mutex_destroy(&mtx_images);
    pthread_mutex_destroy(&mtx_logout);

    // tanquem els file descriptors
    close(0);
    close(1);
    close(2);

    // Tornem el CTRL+C a la funcio original
    signal(SIGINT, SIG_DFL);

    // Ens despedim i tirem un SIGINT per parar l'execucio
    raise(SIGINT);
}

// Allibera la info del fitxer de configuracio
void ATR_free_configfile()
{
    free(g_config.ip);
    free(g_config.path);
}

// Alliberar la memoria de la llista d'usuaris
void ATR_free_userlist()
{
    int i;
    for (i = 0; i < g_num_users; i++)
    {
        free(g_list_users[i].name);
        free(g_list_users[i].code);
    }
    free(g_list_users);
}

// Tanca els file descriptors dels sockets
void ATR_close_socketsfd()
{
    int i;
    for (i = 0; i < g_num_clients; i++)
    {
        close(g_list_fd_clients[i]);
    }
    close(g_fd_server);
    free(g_list_fd_clients);
}

// Cancela els threads que estiguin actius
void ATR_cancel_threads()
{
    int res;

    // Cancelem tots els threads restants
    for (int i = 0; i < g_num_clients; i++)
    {
        res = (int)pthread_cancel(g_list_threads[i]);
        if (res != 0)
        {
            print(ERROR_THREAD_CANCEL);
        }
        /*res = (int)pthread_join(g_list_threads[i], NULL);
        if (res != 0)
        {
            print(ERROR_THREAD_CANCEL);
        }*/
    }
    free(g_list_threads);
}

// Imprimim per pantalla el que ens enviin pero controlat amb mutex
void print(char *buffer)
{
    // Bloquejem el recurs global per printar
    ATR_mutex_lock(mtx_print);

    // Printem
    write(1, buffer, strlen(buffer));

    // Desbloquejem el recurs per printar
    ATR_mutex_unlock(mtx_print);
}