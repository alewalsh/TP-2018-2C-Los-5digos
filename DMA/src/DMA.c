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

int main(int argc, char ** argv){
	t_log * logger = log_create("DAM.log", "DAM", true, LOG_LEVEL_INFO);
	t_socket * socket = inicializarTSocket(1, logger);
	config = cargarConfiguracion("config.cfg", DAM, logger);

	puts(config->ipFM9);
	puts(config->ipMDJ);

	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */

	return EXIT_SUCCESS;
}
