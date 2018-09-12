/*
 * consola.h
 *
 *  Created on: 9 sep. 2018
 *      Author: utnso
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <commons/string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "funcionesConsola.h"
#include <javaStrings.h>
#include <commons/collections/list.h>

//TODO: Verificar si este import esta bien o si esta mal, el cuatri pasado no lo puse aca
// pero ahora si no lo pongo no me toma los case
//TODO: Probar comentando el include este
#include <structCommons.h>

void mainConsola();
void consolePrintHeader();

#endif /* CONSOLA_H_ */
