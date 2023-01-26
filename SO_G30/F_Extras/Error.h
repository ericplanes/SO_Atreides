// Codigos de error
#define ERROR_CODE 500
#define ERROR_CONF_MISSING "ERROR: Configuration text name missing\n"

#define ERROR_PARAM_EXCEEDED "ERROR: The number of parameters has been exceeded.\n"
#define ERROR_PARAM_UNREACHED "ERROR: The number of parameters hasn't been reached.\n"
#define ERROR_LENGTH_EXCEEDED "ERROR: The length of the file name is bigger than expected.\n"
#define ERROR_FORK_CREAT "ERROR: The fork hasnt been successfully created.\n"
#define ERROR_CONF_UNEXISTENT "ERROR: The configuration text does not exist\n"
#define ERROR_COM_NOT_FOUND "ERROR: Command not found, please try again.\n"
#define ERROR_MD5SUM "ERROR: md5sum couldn't be done properly.\n"
#define ERROR_FSTAT "ERROR: fstat wasn't successfuly done.\n"
#define ERROR_FILE_O "ERROR: There was an error opening a file.\n"
#define ERROR_FORMAT_IMAGE "ERROR: The image must be .jpg in order to be sended\n"
#define ERROR_THREAD_CANCEL "ERROR: Thread couldn't be canceled properly.\n"

// Client error strings
#define CERR_INV_PORT "ERROR: The port number of the configuration file is incorrect.\n"
#define CERR_SOCKET_CREAT "ERROR: The socket of the client couldn't be created\n"
#define CERR_IP "ERROR: There is an error with the IP.\n"
#define CERR_SEARCH_REC "ERROR: The code we recieved from the server isn't correct.\n"
#define CERR_SEARCH_RESP "ERROR: The format of the answer given by the server isn't correct\n"
#define CERR_SEARCH_FORM "ERROR: The information sent wasn't correctly formated.\n"
#define CERR_NOT_CON "ERROR: You are not connected to Atreides.\n"
#define CERR_UNKNOWN_RESP "ERROR: The code recieved from the server is unknown.\n"
#define CERR_UNKNOWN_SEND "ERROR: The code sended to the server is unknown.\n"
#define CERR_SEND_RESP "ERROR: The server had an error with the message sent.\n"

// Errors del servidor
#define SERR_SOCKET_CREAT "ERROR: Socket creation went wrong.\n"
#define SERR_BIND "ERROR: An error ocurred while binding the port.\n"
#define SERR_LISTEN "ERROR: Couldn't start listening for other sockets correctly.\n"
#define SERR_THREAD_CREAT "ERROR: Thread hasn't been created successfuly\n"
#define SERR_THREAD_DETACH "ERROR: Thread detach hasn't been successful\n"
#define SERR_MUT_LOCK "ERROR: Error at locking the thread with the function 'pthread_mutex_lock'\n"
#define SERR_MUT_UNLOCK "ERROR: Error at unlocking the thread with the function 'pthread_mutex_unlock'\n"
#define SERR_LOGIN "ERROR: There was an error attempting to LogIn on Atreides.\n"
#define SERR_DATA_FORMAT "ERROR: Data format recieved was wrong\n"
#define SERR_SEARCH_RESP "ERROR: There was an error while parsing the information to send.\n"
#define SERR_MODIFY_USER "ERROR: There was an error modifying the user. Not found on the file\n"
#define SERR_USERS_FILE "ERROR: User data file wasn't opened or created correctly\n"
#define SERR_UNKNOWN_RESP "ERROR: The code recieved from the client is unknown\n"
#define SERR_PHOTO_CORRUPTED "ERROR: The photo was corrupted while being sent\n"
#define SERR_UNEXISTENT_ID "ERROR: The id asked doesn't exists\n"