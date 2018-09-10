/*
 ============================================================================
 Name        : DMA.c
 Author      : Franco Lopez
 Version     :
 Copyright   : Your copyright notice
 Description : Proyecto destinado al DMA
 ============================================================================
 */

#include "DMA.h"

int main(){
	t_log * logger = log_create("DAM.log", "DAM", true, LOG_LEVEL_INFO);
	t_socket * socket = inicializarTSocket(1, logger);

	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */

	return EXIT_SUCCESS;
}
