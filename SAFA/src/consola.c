/*
 * consola.c
 *
 *  Created on: 9 sep. 2018
 *      Author: utnso
 */

#include "consola.h"
#include <grantp/mutex_log.h>


extern t_log_mutex *logger;
int seguirEjecutando = 1;


void executeCommand(int comando, char *args) {
    switch (comando) {
        case EJECUTAR:
            consolaEjecutar(args);
            break;
        case STATUS:
        	if (args != NULL){
	            consolaStatusDTB(args);}
			else {
	            consolaStatus();}
            break;
        case FINALIZAR:
            consolaLiberar();
//            consoleBlock(args);
//            printf("Comando FINALIZAR seleccionado.\n");
            break;
        case METRICAS:
        	if (args != NULL){
                consolaMetricasDTB(args);}
        	else {
                consolaMetricas();}
            break;
        case EXIT:
        	seguirEjecutando = 0;
            consoleExit();
            break;
        case HELP:
            printf("Comando HELP seleccionado.\n");
            consoleHelp();
            break;
        case CLEAR:
            consoleClear();
            break;
        default:
            printf("El comando igresado no es valido.\n");
//            consoleHelp();
            break;
    }
}


void mainConsola() {

    char *rawline;
    char *comando;
	char *args;

    consolePrintHeader();

    while (seguirEjecutando) {

    	rawline = readline("\n➜  ~ ");
    	string_trim_left(&rawline);

    	if (rawline) {

            //agrego la linea al historial
            add_history(rawline);

            if (strlen(rawline)) {
            	string_trim_right(&rawline);

                //Parseo el comando
                parseCommand(rawline, &comando, &args);

                //obtengo el id del comando
                int id = getIdFunction(comando);

                //lo ejecuto
                executeCommand(id, args);
            }

        }
    	if(strlen(rawline)){
    		freeCommand(comando, args);
			free(rawline);
    	}

    }
}


void consolePrintHeader() {
    // impresion por pantalla de la cabecera de consola
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("/home/utnso/git/tp-2018-2c-Los-5digos/SAFA/logo.txt", "r");
    if (fp == NULL) {
        log_error_mutex(logger, "Error al abrir el archivo: ");
    } else {
        while ((read = getline(&line, &len, fp)) != -1) {
            printf("%s", line);
        }
    }
    free(line);
    printf("\n");
    printf("Para consultar comandos de consola escriba 'HELP'\n");
    printf("\n");
}
