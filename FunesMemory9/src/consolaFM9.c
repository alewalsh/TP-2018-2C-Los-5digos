/*
 * consolaFM9.c
 *
 *  Created on: 22 nov. 2018
 *      Author: utnso
 */

#include "consolaFM9.h"

void manejarConsolaFM9()
{
	printf("Se esta ejecutando la consola del Funes Memory \n");

	char * token;
	char * linea;
	char * comandoSpliteado[2];
	int i = 0;

	while (1) {

		linea = readline(">");

		if (linea)
			add_history(linea);

		if (strcmp(linea, "exit") == 0) {
			free(linea);
			break;
		}
	}

	token = strtok(linea," ");

	while(token != NULL){
		comandoSpliteado[i] = token;
		token = strtok(NULL," ");
		i++;
	}


	if(strcmp(comandoSpliteado[0],"dump") == 0){
		int esIdProceso = esUnNumeroIDProceso(comandoSpliteado[1]);

		if(esIdProceso == 0){

			ejecutarDumpSegunEsquemaMemoria(storage);

		}else{

			log_info_mutex(logger,"El comando ingresado no es correcto \n");
		}

	}

}

void ejecutarDumpSegunEsquemaMemoria(char * storage){

	switch(config->modoEjecucion){

		case SEG:
			dumpEnSegmentacionPura(storage);
			break;

		case TPI:
			dumpEnTPI(storage);
			break;

		case SPA:
			dumpEnSegPag(storage);
			break;
	}
}

void dumpEnSegmentacionPura(char * storage){
	//logica dump en segmentacion
}

void dumpEnTPI(char * storage){
	//logica dump en TPI
}

void dumpEnSegPag(char * storage){
	//logica dump en seg. paginada
}

int esUnNumeroIDProceso(char * numero){
	int longitud = strlen(numero);
	int i;
	for(i = 0;i<longitud;i++){

		if(isdigit(numero[i])){
			i++;

		}else{

			return 1;

		}

	}

	return 0;
}
