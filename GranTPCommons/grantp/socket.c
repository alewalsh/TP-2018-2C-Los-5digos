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
		case SAFA_CPU_CONNECT:// = 1006,
			return "Conexión SAFA - CPU.";
		case SAFA_CPU_OK:// = 1007,
			return "SAFA - CPU OK.";
		case SAFA_CPU_FAIL:// = 1008,
			return "SAFA - CPU FAIL.";
		case SAFA_CPU_DISCONNECT:// = 1009,
			return "Desconexión SAFA - CPU.";
		case SAFA_CPU_NUEVO_DUMMY:// = 1010,
			return "El SAFA envió un nuevo dummy al CPU.";
		case SAFA_CPU_EJECUTAR:// = 1011,
			return "El SAFA ordenó una ejecución al CPU.";
		case SAFA_CPU_QUANTUM:
			return "El SAFA envió un nuevo quantum al CPU.";
		case DAM_FM9_CONNECT:// = 1013,
			return "Conexión DAM - FM9.";
		case DAM_FM9_OK:// = 1014,
			return "DAM - FM9 OK.";
		case DAM_FM9_FAIL:// = 1015,
			return "DAM - FM9 FAIL.";
		case DAM_FM9_DISCONNECT:// = 1016,
			return "Desconexión DAM - FM9.";
		case DAM_FM9_CARGAR_ESCRIPTORIO:// = 1018,
			return "El DAM ordenó cargar un escriptorio en FM9.";
		case DAM_FM9_ENVIO_PKG:// = 1019,
			return "El DAM envió un paquete para guardar en un escriptorio.";
		case DAM_FM9_FLUSH:
			return "El DAM ordenó la realización del flush de un escriptorio.";
		case DAM_MDJ_CONNECT:// = 1021,
			return "Conexión DAM - MDJ.";
		case DAM_MDJ_OK:// = 1022,
			return "DAM - MDJ OK.";
		case DAM_MDJ_FAIL:// = 1023,
			return "DAM - MDJ FAIL.";
		case DAM_MDJ_DISCONNECT:// = 1024,
			return "Desconexión DAM - MDJ.";
		case DAM_MDJ_TRANSFER_SIZE:// = 1025,
			return "El DAM envió al MDJ el transfer size.";
		case DAM_MDJ_CONFIRMAR_EXISTENCIA_ARCHIVO:// = 1026,
			return "El DAM pide la confirmación de la existencia del archivo.";
		case DAM_MDJ_CARGAR_ESCRIPTORIO:// = 1027,
			return "El DAM pide la carga de un escriptorio.";
		case DAM_MDJ_HACER_FLUSH:// = 1028,
			return "El DAM manda el flush al MDJ.";
		case DAM_MDJ_GUARDAR_DATOS:// = 1029,
			return "El DAM manda un guardado de datos al MDJ.";
		case DAM_MDJ_CREAR_ARCHIVO:// = 1030,
			return "El DAM ordena la creación de un archivo.";
		case DAM_MDJ_BORRAR_ARCHIVO:// = 1031,
			return "El DAM ordena el borrado de un archivo.";
		case DAM_MDJ_CONFIRMACION_ENVIO_DEL_MDJ:// =
			return "El DAM realiza la confirmacion del envio proviniente del MDJ.";
		case DAM_MDJ_CONFIRMACION_ENVIO_DEL_DMA:
			return "El MDJ realiza la confirmacion del envio proviniente del DAM.";
		case DAM_SAFA_CONNECT:// = 1032,
			return "Conexión DAM - SAFA.";
		case DAM_SAFA_OK:// = 1033,
			return "DAM - SAFA OK.";
		case DAM_SAFA_FAIL:// = 1034,
			return "DAM - SAFA FAIL.";
		case DAM_SAFA_DISCONNECT:// = 1035,
			return "Desconexión DAM - SAFA.";
		case DAM_SAFA_CONFIRMACION_PID_CARGADO:// = 1036,
			return "El DAM envía la confirmación de la carga de un escriptorio al PID correspondiente al SAFA.";
		case DAM_SAFA_CONFIRMACION_SCRIPT_INICIALIZADO:// = 1038,
			return "El DAM envía la confirmación de la inicialización del escriptorio al SAFA.";
		case DAM_SAFA_CONFIRMACION_DATOS_GUARDADOS:// = 1039,
			return "El DAM realiza la confirmación del guardado de datos al SAFA.";
		case DAM_SAFA_CONFIRMACION_CREAR_ARCHIVO:// = 1040,
			return "El DAM realiza la confirmación del archivo creado al SAFA.";
		case DAM_SAFA_CONFIRMACION_BORRAR_ARCHIVO:// = 1041,
			return "El DAM realiza la confirmación del archivo borrado al SAFA.";
		case CPU_FM9_CONNECT:// = 1042,
			return "Conexión CPU - FM9.";
		case CPU_FM9_OK:// = 1043,
			return "CPU - FM9 OK.";
		case CPU_FM9_FAIL:// = 1044,
			return "CPU - FM9 FAIL.";
		case CPU_FM9_DISCONNECT:// = 1045,
			return "Desconexión CPU - FM9.";
		case CPU_FM9_CERRAR_ARCHIVO:// = 1046,
			return "El CPU pide cerrar un archivo en el FM9.";
		case CPU_FM9_ASIGNAR:// = 1047,
			return "El CPU pide guardar una linea en el FM9.";
		case CPU_FM9_CARGAR_ESCRIPTORIO:// = 1048,
			return "El CPU pide cargar un escriptorio en el FM9.";
		case CPU_FM9_FIN_GDT:// = 1049,
			return "El CPU avisa del fin de la ejecución del GDT al FM9.";
		case CPU_FM9_DAME_INSTRUCCION:// = 1050
			return "El CPU pide una instrucción al FM9.";
		case CPU_DAM_CONNECT:// = 1049,
			return "Conexión CPU - DAM.";
		case CPU_DAM_OK:// = 1050,
			return "CPU - DAM OK.";
		case CPU_DAM_FAIL:// = 1051,
			return "CPU - DAM FAIL.";
		case CPU_DAM_DISCONNECT:// = 1052,
			return "Desconexión CPU - DAM.";
		case CPU_DAM_BUSQUEDA_ESCRIPTORIO:// = 1053,
			return "El CPU pide buscar un escriptorio al DAM.";
		case CPU_DAM_ABRIR_ARCHIVO:// = 1054,
			return "El CPU pide abrir un escriptorio al DAM.";
		case CPU_DAM_FLUSH:// = 1055,
			return "El CPU pide realizar el flush de un escriptorio al DAM.";
		case CPU_DAM_CREAR:// = 1056,
			return "El CPU pide crear un escriptorio al DAM.";
		case CPU_DAM_BORRAR:// = 1057,
			return "El CPU pide borrar un escriptorio al DAM.";
		case CPU_SAFA_CONNECT:// = 1049,
			return "Conexión CPU - SAFA.";
		case CPU_SAFA_OK:// = 1050,
			return "CPU - SAFA OK.";
		case CPU_SAFA_FAIL:// = 1051:
			return "CPU - SAFA FAIL.";
		case CPU_SAFA_DISCONNECT:// = 1052:
			return "Desconexión CPU - SAFA.";
		case CPU_SAFA_BLOQUEAR_DUMMMY:// = 1058:
			return "El CPU pide al SAFA bloquear el dummy.";
		case CPU_SAFA_WAIT_RECURSO:// = 1059:
			return "El CPU pide al SAFA realizar un wait de un recurso.";
		case CPU_SAFA_SIGNAL_RECURSO:// = 1060:
			return "El CPU pide al SAFA realizar un signal de un recurso.";
		case CPU_SAFA_BLOQUEAR_DTB:// = 1061:
			return "El CPU pide al SAFA bloquear el DTB.";
		case CPU_SAFA_ABORTAR_DTB:// = 1062:
			return "El CPU pide al SAFA abortar el DTB.";
		case CPU_SAFA_ABORTAR_DTB_NUEVO:// = 1062:
			return "El CPU pide al SAFA abortar el DTB proviniente de NEW.";
		case CPU_SAFA_FIN_EJECUCION_DTB:// = 1063:
			return "El CPU pide al SAFA finalizar la ejecución del DTB.";
		case CPU_SAFA_FIN_EJECUCION_X_QUANTUM_DTB:// = 1064
			return "El CPU pide al SAFA finalizar la ejecución del DTB por fin de quantum.";
		case FM9_CPU_CONNECT:// = 1049:
			return "Conexión FM9 - CPU.";
		case FM9_CPU_OK:// = 1050:
			return "FM9 - CPU OK.";
		case FM9_CPU_FAIL:// = 1051:
			return "FM9 - CPU FAIL.";
		case FM9_CPU_DISCONNECT:// = 1052:
			return "Desconexión FM9 - CPU.";
		case FM9_CPU_ACCESO_INVALIDO:// = 1065:
			return "El FM9 informa al CPU de un acceso invalido.";
		case FM9_CPU_ERROR:// = 1066:
			return "El FM9 informa al CPU de un error inesperado.";
		case FM9_CPU_MEMORIA_INSUFICIENTE:// = 1067:
			return "El FM9 informa al CPU que no hay suficiente memoria para la petición.";
		case FM9_CPU_FALLO_SEGMENTO_MEMORIA:// = 1068:
			return "El FM9 informa al CPU que hubo un fallo en un segmento de memoria.";
		case FM9_CPU_PROCESO_INEXISTENTE:// = 1069:
			return "El FM9 informa al CPU que no existe el proceso indicado.";
		case FM9_CPU_LINEA_GUARDADA:// = 1070:
			return "El FM9 informa al CPU que se ha guardado la linea indicada.";
		case FM9_CPU_ERROR_LINEA_GUARDADA:// = 1071:
			return "El FM9 informa al CPU que hubo un error en el guardado de la linea indicada.";
		case FM9_CPU_ARCHIVO_CERRADO:// = 1072:
			return "El FM9 informa al CPU que el archivo fue cerrado exitosamente.";
		case FM9_CPU_GDT_FINALIZADO:
			return "El FM9 informa al CPU que el GDT fue finalizado exitosamente.";
		case FM9_CPU_DEVUELVO_LINEA:// = 1070:
			return "El FM9 informa al CPU que devolvió la linea pedida exitosamente.";
		case FM9_DAM_MEMORIA_INSUFICIENTE:// = 1073:
			return "El FM9 informa al DAM que la memoria es insuficiente para cumplir la peticion.";
		case FM9_DAM_MEMORIA_INSUFICIENTE_FRAG_EXTERNA:// = 1074:
			return "El FM9 informa al DAM que la memoria es insuficiente para cumplir la peticion por fragmentación externa.";
		case FM9_DAM_ARCHIVO_INEXISTENTE:// = 1075:
			return "El FM9 informa al DAM que no existe el archivo indicado.";
		case FM9_DAM_PROCESO_INEXISTENTE:// = 1076:
			return "El FM9 informa al DAM que no existe el proceso indicado.";
		case FM9_DAM_ERROR_PAQUETES:// = 1077:
			return "El FM9 informa al DAM que hubo un error con los paquetes.";
		case FM9_DAM_ESCRIPTORIO_CARGADO:// = 1078:
			return "El FM9 informa al DAM que se cargó el escriptorio correctamente.";
		case FM9_DAM_ERROR_CARGA_ESCRIPTORIO:// = 1079:
			return "El FM9 informa al DAM que hubo un error al cargar el escriptorio.";
		case FM9_DAM_FLUSH:// = 1080:
			return "El FM9 informa al DAM que se realizó el flush.";
		case FM9_DAM_ERROR_FLUSH:// = 1081:
			return "El FM9 informa al DAM que hubo un error al realizar el flush.";
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

