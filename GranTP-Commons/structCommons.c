/*
 * structCommons.c
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#include "structCommons.h"


void freeEsiInstruction(void *esi) {
    t_esi_instruction *e = (t_esi_instruction *) esi;
    free(e->operation);
    free(e->key);
    free(e->value);
    free(e);
}


void freeEsi(void *esi) {
    t_node_esi *esi1 = (t_node_esi *) esi;
    free(esi1->id);
    free(esi1);
}


void freeBlockedKey(void *bk) {
    t_blocked_keys *bk1 = (t_blocked_keys *) bk;
    free(bk1->esiId);
    free(bk1->key);
    free(bk1);
}
