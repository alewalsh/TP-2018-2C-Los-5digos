/*
 * consola.c
 *
 *  Created on: 4/6/2017
 *      Author: Martin Gauna
 */

#include "comandos.h"

int readCommand (char* buff) {

	// si hay algo en el buffer de la consola se borra.
	if (fgets (buff, TAMANIO_LINEA, stdin) == NULL){
		return NO_INPUT;
	}
	fflush (stdin);
	//if (buff[strlen(buff)-1] != '\n') {
		//si hay chars extra se descartan
		//while (((ch = getchar()) != '\n') && (ch != EOF));
	//}
	// saco el \n.
	if(strlen(buff)) {
		buff[strlen(buff)-1] = '\0';
	}

	return EXIT_SUCCESS;
}


