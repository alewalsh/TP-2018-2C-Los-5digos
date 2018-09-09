/*
 ============================================================================
 Name        : CPU.c
 Author      : Alejandro Walsh
 Version     : 1.0.1
 Copyright   :
 Description : Módulo CPU del Gran TP (Sistemas Operativos, UTN FRBA, 2018)
 ============================================================================
 */
#include "CPU.h"

int main(int argc, char ** argv) {
	t_log * logger = log_create("CPU.log", "CPU", true, LOG_LEVEL_INFO);
	config = cargarConfiguracion(argv[1], CPU, logger);

	printf("%s",config->ipDAM);

	return EXIT_SUCCESS;
}
