/*
 * structCommons.h
 *
 *  Created on: 14 abr. 2018
 *      Author: utnso
 */

#ifndef STRUCTCOMMONS_H_
#define STRUCTCOMMONS_H_

#include <stdint.h>
#include <stdlib.h>

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

enum typeLIST {
    READY = 2001, BLOCKED = 2002, EXECUTING = 2003, FINISHED = 2004, BLOCKEDKEYS
};

enum typeScheduler {
    RR = 1, VRR, PROPIO
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
    STOP = 1, PLAY, BLOCK, UNBLOCK, LIST, STATUS, KILL, DEADLOCK, CLEAR, MAN, EXIT, HELP, ESIS, KEYS
};

void freeEsiInstruction(void *esi);

void freeEsi(void *esi);

void freeBlockedKey(void *bk);

#endif
