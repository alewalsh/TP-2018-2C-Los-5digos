/*
 * comandos.h
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#ifndef COMANDOS_H_
#define COMANDOS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TAMANIO_LINEA 4096
#define NO_INPUT -2
#define TOO_LONG -3


int readCommand (char* prompt);

#endif /* COMANDOS_H_ */
