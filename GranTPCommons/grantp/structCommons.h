/*
 * structCommons.h
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#ifndef STRUCTCOMMONS_H_
#define STRUCTCOMMONS_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 *  Ejemplo DTB: habria que ponerlo en GranTPCommons si utilizamos este.
 */
typedef struct {
	int idGDT;
    char *dirEscriptorio;
    int programCounter;
    bool flagInicializado;
    char *tablaDirecciones;
    int cantidadLineas;
} t_dtb;

typedef struct {
    char *operation;
    char *key;
    char *value;
} t_esi_instruction;

typedef struct {
    char *inputs;
    char *size;
} t_init_instance;

typedef struct {
    char *id;
    int socket;
    double estimate;
    int real;
    int time;
    int shouldEstimate;
    uint16_t status;
} t_node_esi;

typedef struct {
    char *esiId;
    char *key;
    uint16_t status;
} t_blocked_keys;

enum estadoSAFA {
	Operativo = 0, Corrupto = 1
};

enum typeLIST {
    READY = 2001, BLOCKED = 2002, EXECUTING = 2003, FINISHED = 2004, BLOCKEDKEYS
};

enum tipoPlanificacion {
    RR = 1, VRR, PROPIO
};

enum tipoEjecucion {
	SEG = 1, TPI, SPA
};

typedef struct {
    uint16_t type;
    int socket;
} t_plan_socket;

enum typeSocket {
    ESI_SOCKET, COORDINATOR_SOCKET
};

enum typeStatus {
    TAKEN = 3001, WAITING = 3002
};

enum command {
    EJECUTAR = 1, STATUS, FINALIZAR, METRICAS, EXIT, HELP, CLEAR
};
//LIST, STATUS, KILL, DEADLOCK, CLEAR, MAN, ESIS, KEYS

void freeEsiInstruction(void *esi);

void freeEsi(void *esi);

void freeBlockedKey(void *bk);

#endif /* COMANDOS_H_ */
