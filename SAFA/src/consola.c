/*
 * consola.c
 *
 *  Created on: 9 sep. 2018
 *      Author: utnso
 */

#include "consola.h"
#include <mutex_log.h>


extern t_log_mutex *logger;
int seguirEjecutando = 1;


void executeCommand(int comando, char *args) {
    switch (comando) {
        case EJECUTAR:
//            consolePlay(args);
            printf("Comando EJECUTAR (%d) seleccionado.\n", comando);
            break;
        case STATUS:
//            consoleStop();
            printf("Comando STATUS seleccionado.\n");
            break;
        case FINALIZAR:
//            consoleBlock(args);
            printf("Comando FINALIZAR seleccionado.\n");
            break;
        case METRICAS:
//            consoleUnblock(args);
            printf("Comando METRICAS seleccionado.\n");
            break;
        case EXIT:
            printf("Comando EXIT seleccionado.\n");
        	seguirEjecutando = 0;
            consoleExit();
            break;
        case HELP:
            printf("Comando HELP seleccionado.\n");
            consoleHelp();
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

        if (rawline) {

            //agrego la linea al historial
            add_history(rawline);

            if (strlen(rawline)) {

                //Parseo el comando
                parseCommand(rawline, &comando, &args);

                //obtengo el id del comando
                int id = getIdFunction(comando);

                //lo ejecuto
                executeCommand(id, args);
            }
        }

        freeCommand(comando, args);
        free(rawline);
    }
}


void consolePrintHeader() {
    // impresion por pantalla de la cabecera de consola
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("logo.txt", "r");
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


