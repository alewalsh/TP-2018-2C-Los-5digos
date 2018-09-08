/*
 ============================================================================
 Name        : SAFA.c
 Author      : Alejandro Walsh
 Version     : 1.0
 Copyright   :
 Description : Módulo SAFA para el TP de SO 2c 2018
 ============================================================================
 */

#include "SAFA.h"

int main(void) {
	t_log * logger = log_create("SAFA.log", "SAFA", true, LOG_LEVEL_INFO);
	t_socket * socket = inicializarTSocket(1, logger);

	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}
