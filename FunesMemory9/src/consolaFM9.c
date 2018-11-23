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


	if(strcmp(comandoSpliteado[0],"dump") == 0)
	{
		int esIdProceso = esUnNumeroIDProceso(comandoSpliteado[1]);

		if(esIdProceso == 0){

			ejecutarDumpSegunEsquemaMemoria(comandoSpliteado[1]);

		}else{

			log_info_mutex(logger,"El comando ingresado no es correcto \n");
		}

	}

}

void ejecutarDumpSegunEsquemaMemoria(char * pidString)
{
	int pid = strtol(pidString, NULL, 10);
	if (pid != 0)
	{
		t_datosFlush * datos = malloc(sizeof(t_datosFlush));
		datos->pid = pid;
		switch(config->modoEjecucion){

			case SEG:
				flushSegmentacion(0, datos, DUMP);
				break;
			case TPI:
				flushTPI(0, datos, DUMP);
				break;
			case SPA:
				flushSegmentacionPaginada(0, datos, DUMP);
				break;
			default:
				printf("Modo de ejecución inexistente, verifique el archivo de configuracion y vuelva a iniciar el proceso.");
				log_warning_mutex(logger, "Modo de ejecución inexistente, verifique el archivo de configuracion y vuelva a iniciar el proceso.");
				break;
		}
		free(datos);
	}
	else
	{
		printf("PID erróneo, por favor verifique la información ingreasada por consola.");
		log_error_mutex(logger, "PID erróneo, por favor verifique la información ingreasada por consola.");
	}
}

//void dumpEnSegmentacionPura(int pid){
//	//logica dump en segmentacion
//}

void dumpEnTPI(int pid){
	//logica dump en TPI
}

void dumpEnSegPag(int pid){
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
