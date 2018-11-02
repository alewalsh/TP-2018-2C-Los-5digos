/*
 * socket.c
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#include "socket.h"

/**
 *
 */
t_socket *inicializarTSocket(int socket, t_log *logger) {
    t_socket *sock = (t_socket *) malloc(sizeof(t_socket));
    sock->socket = socket;
    if (pthread_mutex_init(&(sock->mutex), NULL)) {
        log_error(logger, "Error al inicializar mutex para t_socket");
    }
    return sock;
}

/**
 * @NAME: escuchar
 * @DESC: crea un socket y se encarga de hacer un liste sobre el mismo.
 * @PARAMS: {int} 	puerto Numero de puerto
 * 			{int *}	socket Puntero en el que se almacenara el socket, necesita tener la memoria asignada.
 * 			{t_log*} Logger.
 */
int escuchar(int puerto, int *socket, t_log *logger) {
    int listenBacklog = BACKLOG;

    if (cargarSocket(puerto, "", socket, logger)) {
        return EXIT_FAILURE;
    }
    if (listen(*socket, listenBacklog) < 0) {
        log_error(logger, "Error en el listen: %s", strerror(errno));
        close(*socket);
        return EXIT_FAILURE;
    }
    log_debug(logger, "Escuchando en el puerto: %d", puerto);
    return EXIT_SUCCESS;
}

/**
 * @NAME: aceptar
 * @DESC: Acepta una conexion sobre un socket que esta haciendo un listen. por lo que hay que llamar a escuchar antes que a esta funcion.
 * @PARAMS: {int} 	socket Socket que esta escuchando
 * 			{int *}	newSocket Nuevo sokcet generado al hacer el accept.
 * 			{t_log*} Logger.
 */
int aceptar(int socket, int *newSocket, t_log *logger) {
    struct sockaddr_storage their_addr;
    socklen_t addrSize;
    addrSize = sizeof their_addr;
    *newSocket = accept(socket, (struct sockaddr *) &their_addr, &addrSize);
    if (*newSocket < 0) {
        log_error(logger, "Error en el accept: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    log_debug(logger, "Conexion aceptada.");
    return EXIT_SUCCESS;
}

/**
 * @NAME: cargarSocket
 * @DESC: Crea un socket que se puede usar tanto para listen, si no se le pasa la ip, como para connect,
 * 		  si le paso la ip a la que me quiero conectar. La memoria de la variables puntero debe estar previamente asignada.
 * @PARAMS: {int} 	iPuerto puesto que se le asigna al socket.
 * 			{int*}	ip ip a la que me quiero conectar, o si es null el socket se puede usar para hacer listen.
 * 			{int*}	pSocket Socket creado dentro de la funcion.
 * 			{t_log*} Logger.
 */
int cargarSocket(int iPuerto, const char *ip, int *pSocket, t_log *logger) {
    int socketFD;
    struct addrinfo hints, *servInfo, *p;
    int rv;
    char *puerto = string_itoa(iPuerto);
    log_trace(logger, "--- cargarSocket ---");
    if (!strcmp(puerto, "")) {
        log_error(logger, "Error al convertir el puerto a string.");
        free(puerto);
        return EXIT_FAILURE;
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (!strcmp(ip, "")) {        // use my IP
        hints.ai_flags = AI_PASSIVE;
        ip = NULL;
    }

    if ((rv = getaddrinfo(ip, puerto, &hints, &servInfo)) != 0) {
        log_error(logger, "getaddrinfo:%s", gai_strerror(rv));
        free(puerto);
        return EXIT_FAILURE;
    }
    for (p = servInfo; p != NULL; p = p->ai_next) {
        if ((socketFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            log_warning(logger, "Socket error: %s", strerror(errno));
            continue;
        }
        int enable = 1;
        if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            log_warning(logger, "Fallo el setsockopt(SO_REUSEADDR)");
        }
        if (ip == NULL) {
            if (bind(socketFD, p->ai_addr, p->ai_addrlen) == -1) {
                close(socketFD);
                log_warning(logger, "Bind error: %s", strerror(errno));
                continue;
            }
        } else {
            if (connect(socketFD, p->ai_addr, p->ai_addrlen) == -1) {
                close(socketFD);
                log_warning(logger, "Connect error: %s", strerror(errno));
                continue;
            }
        }
        break;
    }
    freeaddrinfo(servInfo);
    if (p == NULL) {
        log_error(logger, "No se pudo crear el socket.");
        free(puerto);
        return EXIT_FAILURE;
    }
    *pSocket = socketFD;
    log_trace(logger, "socket - SocketFD: %d", socketFD);
    free(puerto);
    return EXIT_SUCCESS;
}

/**
 * @NAME: enviarHandshake
 * @DESC: Envia un codigo de handshake,luego recibe la respuesta y chequea que sea la esperada.
 * 		  NOTA IMPORTANTE: no tengo que llamar a recibirHandshake,
 * @PARAMS: {int} 	socket por el que realizo la comunicación.
 * 			{uint16_t}	codigoMio Codigo del programa que llama a la función.
 * 			{uint16_t}	codigoOtro Codigo del programa  al que me quiero conectar.
 * 			{t_log*} Logger.
 */
int enviarHandshake(int socket, uint16_t codigoMio, uint16_t codigoOtro, t_log *logger) {
    t_package handshakeRcv;

    if (enviar(socket, codigoMio, NULL, 0, logger)) {
        log_error(logger, "Error al enviar el handshake");
        return EXIT_FAILURE;
    }
    if (recibir(socket, &handshakeRcv, logger)) {
        log_error(logger, "Error al recibir el handshake");
        return EXIT_FAILURE;
    }
    if (handshakeRcv.code != codigoOtro) {
        log_warning(logger, "Codigo de handshake incorrecto");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * @NAME: recibirHandshake
 * @DESC: Recibe un handshake y manda correspondiente la respuesta.
 * 		  NOTA IMPORTANTE: no tengo que llamar a enviarHandshake antes de usar esta funcion.
 * @PARAMS: {int} 	socket por el que realizo la comunicación.
 * 			{uint16_t}	codigoMio Codigo del programa que llama a la función.
 * 			{uint16_t}	codigoOtro Se guarda el codigo de la persona que mandó el handshake.
 * 			{t_log*} Logger.
 */
int recibirHandshake(int socket, uint16_t codigoMio, uint16_t *codigoOtro,
                     t_log *logger) {
    t_package handshakeRcv;
    if (recibir(socket, &handshakeRcv, logger)) {
        log_error(logger, "Error al recibir el handshake.");
        return EXIT_FAILURE;
    }
    *codigoOtro = handshakeRcv.code;
    if (enviar(socket, codigoMio, NULL, 0, logger)) {
        log_error(logger, "Error al enviar el handshake");
        return EXIT_FAILURE;
    }
    log_debug(logger, "Codigo de Handshake recibido: %s.", codigoIDToString(handshakeRcv.code));
    return EXIT_SUCCESS;
}

/**
 * @NAME: packageSize
 * @DESC: Devuelve el tamaño del tipo t_package junto con el tamaño del contenido de data.
 * @PARAMS: {uint32_t} size Tiene que ser el tamaño del contenido de data.
 */
uint32_t packageSize(uint32_t size) {
    return size + packageHeaderSize;
}

/**
 * @NAME: compress
 * @DESC: Se encarga de crear un bloque de memoria los diferentes datos de un mensaje en un solo bloque de memoria.
 * 		  NOTA: Funcion de uso interna.
 * @PARAMS: {int} 	code Código a enviar en el paquete.
 * 			{char*}	data Datos a enviar.
 * 			{uint32_t} size Tamaño del contenido de 'data'.
 * 			{t_log*} Logger.
 */
char *compress(int code, char *data, uint32_t size, t_log *logger) {
    char *compressPack = (char *) malloc(packageSize(size));
    if (compressPack != NULL) {
        memcpy(compressPack, &code, sizeof(uint16_t));
        memcpy(compressPack + sizeof(uint16_t), &size, sizeof(uint32_t));
        memcpy(compressPack + packageSize(0), data, size);
        return compressPack;
    }
    free(compressPack);
    log_error(logger, "Error al asignar espacio para el packete de salida");
    return NULL;
}

/**
 * @NAME: enviar
 * @DESC: Se encar de enviar un mensaje, con el codigo y los datos con las que es llamado.
 * 		  NOTA: el proceso al que le envio los datos tiene que estar esperando con un recibir.
 * @PARAMS: {int} 	socket Código a enviar en el paquete.
 * 			{uint16_t}	code Codigo de operación.
 * 			{char*} data datos que se quieren enviar. El payload.
 * 			{uint32_t} size Tamaño de lo que voy enviar, tamaño del payload.
 * 			{t_log*} Logger.
 */
int enviar(int socket, uint16_t code, char *data, uint32_t size, t_log *logger) {
    log_trace(logger, "Enviar()");
    char *package = compress(code, data, size, logger);
    if(package == NULL){
        EXIT_FAILURE;
    }
    int sizeOfData = packageSize(size);
    int totalDataSent = 0;
    int sent;
    log_trace(logger, "Codigo:%s\tSize:%d", codigoIDToString(code), size);
    do {
        sent = send(socket, (void *) package, packageSize(size) - totalDataSent, MSG_NOSIGNAL);
        if (sent < 0) {
            log_error(logger, "Error al enviar: %s", strerror(errno));
            free(package);
            return EXIT_FAILURE;
        }
        totalDataSent += sent;
    } while (totalDataSent < sizeOfData);
    log_debug(logger, "Send finished");
    free(package);
    return EXIT_SUCCESS;
}

int enviar_m(int socket, uint16_t code, char *data, uint32_t size, t_log_mutex *logger) {
    int res;
    log_lock(logger);
    res = enviar(socket, code, data, size, logger->logger);
    log_unlock(logger);
    return res;
}

/**
 * @NAME: recibir
 * @DESC: Recibe un mensaje y lo guarda en el segundo parametro, el t_package*.
 * 		  NOTA: Se queda a la espera de que el otro proceso realize un enviar.
 * @PARAMS: {int} 	socket Socket por el que se hace la comunicación.
 * 			{t_package*} mensaje donde se guarda el mensaje recibido.
 * 			{char*} data datos que se quieren enviar. El payload.
 * 			{t_log*} Logger.
 */
int recibir(int socket, t_package *mensaje, t_log *logger) {
    int headerSize = packageHeaderSize;
    char *buffer;
    //Inicializo el puntero en null, en caso de que no sea null se que tengo que liberarlo.
    mensaje->data = NULL;
    //Recibo el header primero
    if (recvPkg(socket, &buffer, headerSize, logger)) {
        return EXIT_FAILURE;
    }
    // Si buff null y exit_success recibi 0 entonces devuelvo disconnect
    if (buffer == NULL) {
        mensaje->code = SOCKET_DISCONECT;
        mensaje->size = 0;
        mensaje->data = NULL;
        return EXIT_SUCCESS;
    }
    //descomprimo el header.
    memcpy(&(mensaje->code), buffer, sizeof(uint16_t));
    memcpy(&(mensaje->size), buffer + sizeof(uint16_t), sizeof(uint32_t));
    log_debug(logger, "Header Recibido - Code:%s\tCodeNumber:%d\tSize:%d", codigoIDToString(mensaje->code),
              mensaje->code, mensaje->size);
    if (buffer) {
        free(buffer);
    }
    if (!mensaje->size) {
        return EXIT_SUCCESS;
    }
    //Ahora recibo los datos de ser necesario.
    if (recvPkg(socket, &buffer, mensaje->size, logger)) {
        return EXIT_FAILURE;
    }
    // Si buff null y exit_success recibi 0 entonces devuelvo disconnect
    if (buffer == NULL) {
        mensaje->code = SOCKET_DISCONECT;
        mensaje->size = 0;
        mensaje->data = NULL;
        return EXIT_SUCCESS;
    }
    mensaje->data = buffer;
    log_trace(logger, "recibir successful");
    return EXIT_SUCCESS;
}

int recibir_m(int socket, t_package *mensaje, t_log_mutex *logger) {
    int res;
    log_lock(logger);
    res = recibir(socket, mensaje, logger->logger);
    log_unlock(logger);
    return res;
}

/**
 * @NAME: recvPkg
 * @DESC: Se encar de hacer el send de forma recursiva hasta que se lea el mensaje completo.
 * 		  NOTA: Funcion de uso interna, no usar.
 * @PARAMS: {int} socket Socket por el que se hace la comunicación.
 * 			{char**} buffer Puntero al char* donde se va a guardar el mensaje.
 * 			{uint32_t} size Tamaño de lo que voy recibir, tamaño del payload.
 * 			{t_log*} Logger.
 */
int recvPkg(int socket, char **buffer, uint32_t size, t_log *logger) {
    int recibido, recibidoTotal = 0;
    char *buff, *buffAux;
    *buffer = NULL;

    if (size < 1) {
        return EXIT_SUCCESS;
    }
    buff = (char *) malloc(size);
    if(buff == NULL){
        log_error(logger, "Error en el malloc del recibir: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    buffAux = (char *) malloc(size);
    if(buffAux == NULL){
        free(buff);
        log_error(logger, "Error en el malloc del recibir: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    do {
        recibido = recv(socket, buff, size, 0);
        if (recibido < 0) {
            log_error(logger, "Error al recibir: %s", strerror(errno));
            free(buff);
            free(buffAux);
            return EXIT_FAILURE;
        }
        if (recibido == 0) {
            log_warning(logger, "Se desconecto el socket:%d - Msg:%s", socket, strerror(errno));
            free(buff);
            free(buffAux);
//            uint16_t code = SOCKET_DISCONECT;
//            memcpy(buffAux, &code, sizeof(uint16_t));
//            (*buffer) = buffAux;

            return EXIT_SUCCESS;
        }
        if (recibido > 0) {
            memcpy((buffAux + recibidoTotal), buff, recibido);
        }
        recibidoTotal += recibido;
    } while (recibidoTotal < size);
    free(buff);
    (*buffer) = buffAux;
    return EXIT_SUCCESS;
}

/**
 * @NAME: highestFD
 * @DESC: Compara 2 file descriptors y retorna el valor del mayor más 1 para ser almacenado en el select.
 * @PARAMS: {int} fd File descriptor más grande guardado en el select.
 * 			{int} nfd Nuevo file descriptor.
 */
int highestFD(int fd, int nfd) {
    if (fd >= nfd) {
        return fd + 1;
    }
    return nfd;
}

/**
 * @NAME: freeData
 * @DESC: Libera el campo data de un t_package si es que tiene asignado algo
 * @PARAMS: {t_package} paquete del que deseo liberar el campo data.
 */
void freeData(t_package pkg) {
    if (pkg.size && pkg.data != NULL) {
        free(pkg.data);
    }
}

/**
 * @NAME: freePackage
 * @DESC: Libera un t_package* y su campo data si es que estos estan inicializados. Si el t_package no fue creado con
 * malloc la funcion va a fallar.
 * @PARAMS: {t_package *} Puntero a un t_package creado dinamicamente que deseo liberar..
 */
void freePackage(t_package *pkg) {
    if (pkg != NULL) {
        freeData(*pkg);
        free(pkg);
    }
}

/**
 * @NAME: getIP
 * @DESC: Retorna la ip de la maquina.
 * @PARAMS:
 */
char *getIP() {
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    ifr.ifr_addr.sa_family = AF_INET;

    snprintf(ifr.ifr_name, IFNAMSIZ, "eth0");

    ioctl(fd, SIOCGIFADDR, &ifr);

    char *ip = inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr);

    close(fd);
    return ip;
}

/**
 * @NAME: parseAdress
 * @DESC: Pasea un string del tipo "xxx.xxx.xxx.xxx:puerto" en 2 variables. Un string para la ip y un int para el puerto.
 * @PARAMS: {char *} String que se desea parsear.
 *          {char **} Puntero a char* donde se asignara un nuevo string dinamicamente creado. (El string NO tiene que estar Inicializado).
 *          {int *} puntero a entero donde se guardara el puerto. (Debe de estar inicializado.)
 */
void parseAdress(char *adress, char **ip, int *puerto) {
    char **array = string_split(adress, ":");
    (*puerto) = atoi(array[1]);
    (*ip) = string_duplicate(array[0]);
    free(array);
}


/**
 * @NAME: acceptConnection
 * @DESC: Funcion con la que se aceptanconexiones entrantes y se recibe el handshake.
 * @PARAMS: {int} Socket que esta escuchando conexiones entrantes.
 *          {int *} Puntero a entero donde se guardara el descriptor del nuevo socket que se genera despues del accept. (DEBE ESTAR INICIALIZADO)
 *          {uint16_t } Codigo de handshake a enviar
 *          {uint16_t *} Variable donde se guardara el Codigo de handshake recibido (DEBE ESTAR INICIALIZADO)
 *          {t_log *} logger.
 */
int acceptConnection(int socketListen, int *pNewSocket, uint16_t handshake,
                     uint16_t *handshakeotro, t_log *logger) {

    if (!aceptar(socketListen, pNewSocket, logger)) {
        if (!recibirHandshake(*pNewSocket, handshake, handshakeotro, logger)) {
            return EXIT_SUCCESS;
        }
    }
    log_error(logger, "Error al aceptar una conexion.");
    return EXIT_FAILURE;
}

/**
 * @NAME: codigoIDToString
 * @DESC: Imprime el nombre del codigo (ENUM) que se le pasa.
 * @PARAMS: {uint16_t} ENUM que se usa para la comunicacion entre procesos.

 */
char *codigoIDToString(uint16_t code) {
    switch (code) {
        case SAFA_HSK:
            return "Handshake SAFA";
		case CPU_HSK:
			return "Handshake CPU";
		case FM9_HSK:
			return "Handshake FM9";
		case DAM_HSK:
			return "Handshake DAM";
		case MDJ_HSK:
			return "Handshake MDJ";
//        case INS_COORD_CONNECT:
//            return "INS_COORD_CONNECT";
//        case INS_COORD_OK:
//            return "INS_COORD_OK";
//        case INS_COORD_FAIL:
//            return "INS_COORD_FAIL";
//        case INS_COORD_GET_FAIL:
//            return "INS_COORD_GET_FAIL";
//        case INS_COORD_SET_FAIL:
//            return "INS_COORD_SET_FAIL";
//        case INS_COORD_STORE_FAIL:
//            return "INS_COORD_STORE_FAIL";
//        case INS_COORD_GET_OK:
//            return "INS_COORD_GET_OK";
//        case INS_COORD_SET_OK:
//            return "INS_COORD_SET_OK";
//        case INS_COORD_STORE_OK:
//            return "INS_COORD_STORE_OK";
//        case INS_COORD_COMPACT_OK:
//            return "INS_COORD_COMPACT_OK";
//        case INS_COORD_BEGIN_COMPACT:
//            return "INS_COORD_BEGIN_COMPACT";
//        case COORD_INS_WELCOME :
//            return "COORD_INS_WELCOME";
//        case COORD_INS_OK :
//            return "COORD_INS_OK";
//        case COORD_INS_FAIL :
//            return "COORD_INS_FAIL";
//        case COORD_INS_GET :
//            return "COORD_INS_GET";
//        case COORD_INS_STORE :
//            return "COORD_INS_STORE";
//        case COORD_INS_SET :
//            return "COORD_INS_SET";
//        case COORD_INS_COMPACT :
//            return "COORD_INS_COMPACT";
//        case ESI_PLAN_CONNECT :
//            return "ESI_PLAN_CONNECT";
//        case ESI_PLAN_OK :
//            return "ESI_PLAN_OK";
//        case ESI_PLAN_FAIL :
//            return "ESI_PLAN_FAIL";
//        case ESI_PLAN_FINISH :
//            return "ESI_PLAN_FINISH";
//        case ESI_PLAN_EOF :
//            return "ESI_PLAN_EOF";
//        case PLAN_ESI_WELCOME :
//            return "PLAN_ESI_WELCOME";
//        case PLAN_ESI_PLAY :
//            return "PLAN_ESI_PLAY";
//        case PLAN_ESI_DEAD :
//            return "PLAN_ESI_DEAD";
//        case ESI_COORD_CONNECT :
//            return "ESI_COORD_CONNECT";
//        case ESI_COORD_SENDPKG :
//            return "ESI_COORD_SENDPKG";
//        case COORD_ESI_WELCOME :
//            return "COORD_ESI_WELCOME";
//        case COORD_ESI_OK :
//            return "COORD_ESI_OK";
//        case COORD_ESI_FAIL :
//            return "COORD_ESI_FAIL";
//        case COORD_PLAN_WELCOME :
//            return "COORD_PLAN_WELCOME";
//        case COORD_PLAN_BLOCK :
//            return "COORD_PLAN_BLOCK";
//        case COORD_PLAN_STORE :
//            return "COORD_PLAN_STORE";
//        case CORRD_PLAN_STATUS_FOUND :
//            return "CORRD_PLAN_STATUS_FOUND";
//        case CORRD_PLAN_STATUS_NOT_FOUND :
//            return "CORRD_PLAN_STATUS_NOT_FOUND";
//        case PLAN_COORD_CONNECT :
//            return "PLAN_COORD_CONNECT";
//        case PLAN_COORD_OK :
//            return "PLAN_COORD_OK";
//        case PLAN_COORD_BLOCKED :
//            return "PLAN_COORD_BLOCKED";
//        case PLAN_CORRD_STORE_FAIL :
//            return "PLAN_CORRD_STORE_FAIL";
//        case PLAN_CORRD_STORE_OK :
//            return "PLAN_CORRD_STORE_OK";
//        case PLAN_CORRD_STATUS :
//            return "PLAN_CORRD_STATUS";
        case SOCKET_DISCONECT :
            return "SOCKET_DISCONECT";
        case PING:
            return "PING";
        case PING_OK:
            return "PING_OK";
        default:
            return "Mensaje recibido ";
    }
}

