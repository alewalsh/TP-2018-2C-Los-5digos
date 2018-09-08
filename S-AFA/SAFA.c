/*
 * SAFA.c
 *
 *  Created on: 2 sep. 2018
 *      Author: Alejandro Walsh
 */
#include "SAFA.h"

pthread_attr_t tattr;

int main(int argc, char* argv)
{
//	validaciones_iniciales(argc, argv[1]);
	pthread_t hiloConexiones;
	pthread_create(hiloConexiones, &tattr, (void *) manejarConexiones, NULL);
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGKILL, sig_handler);
	return 0;
}

void manejarConexiones

void sig_handler(int signo)
{
  if (signo == SIGTERM || signo == SIGKILL || signo == SIGINT)
  {
	  log_warning(logger, "Se recibió una señal de finalizacion del proceso.");
	  exit_gracefully(1);
  }
}

void exit_gracefully(int error)
{

	exit(error);
}

