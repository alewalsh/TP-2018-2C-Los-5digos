/*
 * funcionesConsola.c
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */


#include "funcionesConsola.h"
#include <grantp/javaStrings.h>
#include <grantp/structCommons.h>
#include <grantp/mutex_log.h>
#include <grantp/socket.h>
//#include "executer.h"
#include "planificadorLargo.h"
#include <grantp/compression.h>
//#include "blockedKeyFunctions.h"

// ----------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------


int cantComandos = 7;
const char *functions[] = {"EJECUTAR", "STATUS", "FINALIZAR", "METRICAS", "EXIT", "HELP", "CLEAR"};


const char *descriptions[] = {"Ejecutara el Script indicado.",
                              "Detallara los estados de las colas de planificacion.",
                              "Se enviara el proceso indicado a la cola de EXIT, liberando lugar en READY.",
                              "Se brindara informacion de las metricas solicitadas",
                              "SE VA A CERRAR LA CONSOLA!",
							  "I NEED SOMEBODY HELP, NOT JUST ANYBODY HELP, I NEED SOMEONE HEEEEELP!",
                              "Console Clear"};


void consolaEjecutar(char *args) {

    log_info_mutex(logger, "Se solicitara al PLP crear el DTB asociado al nuevo programa.");
    if(consolaNuevoGDT(args)){
        log_error_mutex(logger, "NO SE HA PODIDO CREAR EL DTB CORRESPONDIENTE.");
        return;
    }
	log_info_mutex(logger, "DTB creado satisfactoriamente y enviado a NEW.");
}


void consolaStatus() {

	//TODO: Separar en si tiene argumento o no tiene argumento. Hacer una funcion para printear cada cola

	if(list_size(colaNew) != 0){
		imprimirNEW();
	} else {printf("Cola NEW sin elementos\n");}
	if(list_size(colaReady) != 0){
		imprimirREADY();
	} else{printf("Cola READY sin elementos\n");}
	if(list_size(colaEjecutando) != 0){
		imprimirEJECUTANDO();
	} else{printf("Cola EJECUTANDO sin elementos\n");}
//	if(list_size(colaBloqueados) != 0){
//
//} else{printf("Cola BLOCKED sin elementos\n");}
//	if(list_size(colaExit) != 0){
//
//} else{printf("Cola EXIT sin elementos\n");}

}


void imprimirNEW(){
    pthread_mutex_lock(&mutexNewList);
	int size = list_size(colaNew);
    printf("*---------------------COLA NEW -----------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaNew, i);
        printf("DTB numero: (%d) \n", dtb->idGDT);
    }
    printf("*-----------------------------------------------------*\n\n");
    pthread_mutex_unlock(&mutexNewList);
}

void imprimirREADY(){
	pthread_mutex_lock(&mutexReadyList);
	int size = list_size(colaReady);
    printf("*---------------------COLA READY ---------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaReady, i);
        printf("DTB numero: (%d) \n", dtb->idGDT);
    }
    printf("*-----------------------------------------------------*\n\n");
	pthread_mutex_unlock(&mutexReadyList);
}

void imprimirEJECUTANDO(){
    pthread_mutex_lock(&mutexEjecutandoList);
	int size = list_size(colaEjecutando);
    printf("*---------------------COLA EJECUTANDO -----------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaEjecutando, i);
        printf("DTB numero: (%d) \n", dtb->idGDT);
    }
    printf("*-----------------------------------------------------*\n\n");
    pthread_mutex_unlock(&mutexEjecutandoList);
}


void consolaLiberar(){
	bloquearDummy();
}



//char *compressKey(char *key, int *size) {
//    *size = sizeof(int) + (strlen(key) * sizeof(char));
//    char *compress = (char *) malloc(*size);
//    char *p = compress;
//    copyStringToBuffer(&p, key);
//    return compress;
//}
//
//void consoleBlock(char *args) {
//
//    if (args == NULL) {
//        log_warning_mutex(logger, "Los parametros ingresados son incorrectos");
//        printf("Los parametros ingresados son incorrectos\n");
//        return;
//    }
//
//    char **params = string_split(args, " ");
//    char *key = params[0];
//    char *id = params[1];
//
//    if (key == NULL || id == NULL) {
//        log_warning_mutex(logger, "Los parametros ingresados son incorrectos");
//        printf("Los parametros ingresados son incorrectos\n");
//        return;
//    }
//
//    uint16_t statusKey = WAITING;
//
//    if (!isKeyBlocked(key)) {
//        statusKey = TAKEN;
//    }
//
//    pthread_mutex_lock(&mutexReadyExecute);
//
//    if (moveEsiToBlocked(id)) {
//        log_debug_mutex(logger, "No se pudo mover el esi a bloqueado");
//        printf("No se bloqueo la clave %s por el esi %s \n\n", key, id);
//        pthread_mutex_unlock(&mutexReadyExecute);
//        return;
//    }
//
//    addNewBlockedKey(id, key, statusKey);
//
//    pthread_mutex_unlock(&mutexReadyExecute);
//
//    log_info_mutex(logger, "Se bloqueo la clave %s por el esi %s", key, id);
//    printf("Se bloqueo la clave %s por el esi %s \n\n", key, id);
//    return;
//}
//
//void consoleUnblock(char *key) {
//
//    if (key == NULL) {
//        log_warning_mutex(logger, "Los parametros ingresados son incorrectos");
//        printf("Los parametros ingresados son incorrectos\n");
//        return;
//    }
//
//    if (!isKeyBlocked(key)) {
//        log_debug_mutex(logger, "Estas queriendo desbloquear una key que no esta bloqueada");
//        printf("Estas queriendo desbloquear una key que no esta bloqueada \n\n");
//        return;
//    }
//
//    if (removeBlockedKeyByKey(key)) {
//        log_error_mutex(logger, "No se desbloqueo la clave %s", key);
//        printf("No se desbloqueo la clave %s \n\n", key);
//        return;
//    }
//
//    //remuevo todas las keys que estan en waiting
//    removeAllWaitingKeyByKey(key);
//
//    //Actualizo la tabla de claves bloqueadas y los esis bloqueados
//    updateBlockedKeysTable();
//    log_trace_mutex(logger, "actualize la tabla");
//
//    log_debug_mutex(logger, "Se desbloqueo correctamente la key %s", key);
//    //sem_post(&sem_newEsi);
//    printf("Se desbloqueo correctamente la key %s \n\n", key);
//}
//
//void consoleList(char *key) {
//
//    if (key == NULL) {
//        log_warning_mutex(logger, "Los parametros ingresados son incorrectos");
//        printf("Los parametros ingresados son incorrectos\n");
//        return;
//    }
//
//    t_list *list = getEsiWaitingKey(key);
//    int size = list_size(list);
//
//    if (size > 0) {
//        printf("*---------------------Procesos esperando la key %s---------------------*\n\n", key);
//        int i;
//        for (i = 0; i < size; i++) {
//            char *esi = (char *) list_get(list, i);
//            printf("- %s \n", esi);
//        }
//        printf("*--------------------------------------------------------------------------*\n\n");
//        log_debug_mutex(logger, "Cantidad de esis esperando a la key %s : %d", key, size);
//    } else {
//        printf("No hay ningun esi esperando la key %s \n", key);
//        log_debug_mutex(logger, "No hay ningun esi esperando la key %s \n", key);
//    }
//    //todo:chequear que no explote por outofmemory
//    list_destroy_and_destroy_elements(list, free);
//}
//
//void consoleKill(char *id) {
//
//    if (id == NULL) {
//        log_warning_mutex(logger, "Los parametros ingresados son incorrectos");
//        printf("Los parametros ingresados son incorrectos\n");
//        return;
//    }
//
//    pthread_mutex_lock(&mutexReadyExecute);
//
//    //remuevo los recursos tomados por este esi si es que tiene
//    removeBlockedKeyFromEsiWithId(id);
//
//    //Actualizo la tabla de claves bloqueadas y los esis bloqueados
//    updateBlockedKeysTable();
//
//    //Ahora lo tengo que buscar en las listas y mandarlo a finalizados.
//    t_node_esi *esi = findEsiById(id);
//
//    deleteSocketFromMaster(esi->socket);
//
//    moveEsiToFinished(esi);
//
//    if (enviar(esi->socket, PLAN_ESI_DEAD, NULL, 0, logger->logger)) {
//        log_error_mutex(logger, "No se pudo enviar a morir al esi");
//        pthread_mutex_unlock(&mutexReadyExecute);
//        return;
//    }
//
//    pthread_mutex_unlock(&mutexReadyExecute);
//
//    log_debug_mutex(logger, "Se movio el esi a finalizado");
//
//}

void consoleClear() {
    system("clear");
}

//void consoleStatus(char *key) {
//
//    t_package pkg;
//
//    if (key == NULL) {
//        log_warning_mutex(logger, "Los parametros ingresados son incorrectos");
//        printf("Los parametros ingresados son incorrectos\n");
//        pthread_mutex_unlock(&mutexReadyExecute);
//        return;
//    }
//
//    pthread_mutex_lock(&mutexReadyExecute);
//
//    //lo saco del select
//    deleteSocketFromMaster(coordinator);
//    updateReadset();
//
//    int size;
//    char *keyCompress = compressKey(key, &size);
//
//    if (enviar(coordinator, PLAN_CORRD_STATUS, keyCompress, size, logger->logger)) {
//        log_error_mutex(logger, "No se pudo mandar el mensaje al coordinador");
//        printf("\n Hubo un error al querer consultar esto\n");
//        addNewSocketToMaster(coordinator);
//        pthread_mutex_unlock(&mutexReadyExecute);
//        return;
//    }
//
//    free(keyCompress);
//
//    if (recibir(coordinator, &pkg, logger->logger)) {
//        log_error_mutex(logger, "No se pudo recibir el mensaje al coordinador");
//        printf("\n Hubo un error al querer consultar esto\n");
//        addNewSocketToMaster(coordinator);
//        pthread_mutex_unlock(&mutexReadyExecute);
//        return;
//    }
//    char *buffer = pkg.data;
//    t_list *list = getEsiWaitingKey(key);
//    switch (pkg.code) {
//        case CORRD_PLAN_STATUS_FOUND:
//            log_trace_mutex(logger, "Se encontro la key");
//
//            char *value = copyStringFromBuffer(&buffer);
//            char *instance = copyStringFromBuffer(&buffer);
//            char *instanceNow = copyStringFromBuffer(&buffer);
//
//            printf("*---------------------Key %s---------------------*\n\n", key);
//            printf("Valor de la key: %s \n", value);
//            printf("Instancia en la que se encuentra: %s\n", instance);
//            printf("Instancia actual a la que perteneceria: %s\n", instanceNow);
//            printf("Esis que esperan esta clave: \n");
//            int i;
//            for (i = 0; i < list_size(list); i++) {
//                char *esi = (char *) list_get(list, i);
//                printf("- %s \n", esi);
//            }
//            printf("*------------------------------------------------*\n\n");
//
//            break;
//        case CORRD_PLAN_STATUS_NOT_FOUND:
//            log_trace_mutex(logger, "No se encontro la key");
//            char *instNow = copyStringFromBuffer(&buffer);
//            printf("*---------------------Key %s---------------------*\n\n", key);
//            printf("Valor de la key: No tiene valor todavia \n");
//            printf("Instancia actual a la que perteneceria: %s\n", instNow);
//            printf("Esis que esperan esta clave: \n");
//            int j;
//            for (j = 0; j < list_size(list); j++) {
//                char *esi = (char *) list_get(list, j);
//                printf("- %s \n", esi);
//            }
//            printf("*------------------------------------------------*\n\n");
//            break;
//        default:
//            log_warning_mutex(logger, "Estoy recibiendo un mensaje que no deberia");
//            printf("\n Hubo un error al querer consultar esto\n");
//            break;
//    }
//
//    free(pkg.data);
//
//    //lo vuevo agregar al select
//    addNewSocketToMaster(coordinator);
//    pthread_mutex_unlock(&mutexReadyExecute);
//
//}
//
//void consoleDeadlock() {
//    int i, j;
//    pthread_mutex_lock(&mutexReadyExecute);
//    int e = getAmountEsi();
//    int k = getAmountUniqueKeys();
//    t_list *keys = getUniqueKeys();
//    t_list *esis = getUniqueEsis();
//
//    int alloc[e][k];
//    int request[e][k];
//    int avail[k];
//
//    //cargo la matriz de asignacion
//    for (i = 0; i < e; i++) {
//
//        for (j = 0; j < k; j++) {
//            //tengo que buscar la key con esiid en taken
//            t_node_esi *esi = list_get(esis, i);
//            char *key = list_get(keys, j);
//            t_blocked_keys *bk = getBlockedKeyWithStatus(key, esi->id, TAKEN);
//            if (bk != NULL) {
//                alloc[i][j] = 1;
//            } else {
//                alloc[i][j] = 0;
//            }
//
//        }
//    }
//
//    //cargo la matriz de peticiones
//    for (i = 0; i < e; i++) {
//
//        for (j = 0; j < k; j++) {
//            t_node_esi *esi = list_get(esis, i);
//            char *key = list_get(keys, j);
//            t_blocked_keys *bk = getBlockedKeyWithStatus(key, esi->id, WAITING);
//            if (bk != NULL) {
//                request[i][j] = 1;
//            } else {
//                request[i][j] = 0;
//            }
//        }
//    }
//
//    for (i = 0; i < e; i++) {
//
//        for (j = 0; j < k; j++) {
//            avail[j] = alloc[i][j] - request[i][j];
//        }
//    }
//
//    //detectar deadlock
//    detectDeadlock(e, k, request, alloc, avail, esis);
//    pthread_mutex_unlock(&mutexReadyExecute);
//
//    //liberar recursos;
//    list_destroy(esis);
//    list_destroy(keys);
//    return;
//}
//
//void detectDeadlock(int e, int k, int need[e][k], int alloc[e][k], int avail[k], t_list *esis) {
//    int finish[100], flag = 1, h;
//    int dead[e];
//    int i, j;
//    for (i = 0; i < e; i++) {
//        finish[i] = 0;
//    }
//
//    while (flag) {
//        flag = 0;
//        for (i = 0; i < e; i++) {
//            int c = 0;
//            for (j = 0; j < k; j++) {
//                if ((finish[i] == 0) && (need[i][j] <= avail[j])) {
//                    //aca entro si tengo le puedo dar el recurso al proceso
//                    c++;
//                    if (c == k) {
//                        for (h = 0; h < k; h++) {
//                            //Aca simulo que ejecute el proceso
//                            avail[h] += alloc[i][j];
//                            finish[i] = 1;
//                            flag = 1;
//                        }
//                        if (finish[i] == 1) {
//                            i = e;
//                        }
//                    }
//                }
//            }
//        }
//    }
//    j = 0;
//    flag = 0;
//    //chequeo si me quedo algun finish en 0(nunca se libero el recurso)
//    for (i = 0; i < e; i++) {
//        if (finish[i] == 0) {
//            dead[j] = i;
//            j++;
//            flag = 1;
//        }
//    }
//    if (flag == 1) {
//        printf("\n\nEl sistema esta en deadlock, los esis bloqueados son:\n");
//        for (i = 0; i < e; i++) {
//            t_node_esi *node = (t_node_esi *) list_get(esis, dead[i]);
//            printf("%s\t", node->id);
//        }
//        //super cabeza pero asi me ahorro de crear un char que concatene los esi(si no aprobamos el sabado lo corrijo).
//        log_info_mutex(logger, "Hay deadlock");
//        for (i = 0; i < e; i++) {
//            t_node_esi *node = (t_node_esi *) list_get(esis, dead[i]);
//            log_info_mutex(logger, "Esi en deadlock: %s", node->id);
//        }
//
//    } else {
//        printf("\nEl sistema no esta en deadlock");
//    }
//
//
//}


void consoleHelp() {
    printf("*--------------------------------------------------------------------------*\n\n");
    int i;
    for (i = 0; i < cantComandos; ++i) {
        printf("%s - %s \n", functions[i], descriptions[i]);
    }
    printf("*--------------------------------------------------------------------------*\n\n");
}

//void consoleStatusAllESI() {
//    printf("*--------------------------------------------------------------------------*\n\n");
//    int size = list_size(statusList);
//    int i;
//    for (i = 0; i < size; ++i) {
//        t_node_esi *esi = getEsiFromIndex(statusList, i);
//        printf("%s - %s \n", esi->id, statusToString(esi->status));
//    }
//    printf("*--------------------------------------------------------------------------*\n\n");
//}
//
//void consoleStatusKeys() {
//    printf("*--------------------------------------------------------------------------*\n\n");
//    int size = list_size(blockedKeys);
//    int i;
//    for (i = 0; i < size; ++i) {
//        t_blocked_keys *bk = (t_blocked_keys *) list_get(blockedKeys, i);
//        printf("%s - %s - %s \n", bk->key, bk->esiId, statusKeyToSting(bk->status));
//    }
//    printf("*--------------------------------------------------------------------------*\n\n");
//}
//
//char *statusToString(uint16_t status) {
//    switch (status) {
//
//        case READY:
//            return "READY";
//        case EXECUTING:
//            return "EXECUTING";
//        case BLOCKED:
//            return "BLOCKED";
//        case FINISHED:
//            return "FINISHED";
//        default:
//            return "QUE LO QUE";
//
//    }
//}
//
//char *statusKeyToSting(uint16_t status) {
//    switch (status) {
//        case TAKEN:
//            return "Tomado";
//        case WAITING:
//            return "Esperando";
//        default:
//            return "QUE LO QUE";
//    }
//}
//
//

void consoleExit() {
    setExit();
}

int getIdFunction(char *function) {
    int i;
    for (i = 0; (i < cantComandos) && (strcmp(function, functions[i]) != 0); i++);
    return (i <= cantComandos - 1) ? (i + 1) : -1;
}

void parseCommand(char *line, char **command, char **args) {
    char **lines = string_n_split(line, 2, " ");
    string_trim(&lines[0]);
    *command = lines[0];
    string_to_upper(*command);
    *args = lines[1];
    free(lines);
}

void freeCommand(char *command, char *args) {
    if (command)
        free(command);
    if (args)
        free(args);
}


// ----------------------------------------------------------------------------------------------------------------------
//  FUNCIONES QUE USA EL THREAD
// ----------------------------------------------------------------------------------------------------------------------

int getExit() {
    return shouldExit;
}

void setExit() {
    pthread_mutex_lock(&mutexExit);
    shouldExit = 1;
    pthread_mutex_unlock(&mutexExit);
}
