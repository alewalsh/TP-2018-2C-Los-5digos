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
//            consoleStop();
            printf("Comando EJECUTAR seleccionado.\n");
            break;
        case STATUS:
//            consolePlay();
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
//            seguirEjecutando = 0;
//            consoleExit();
            printf("Comando EXIT seleccionado.\n");
            break;
        case HELP:
            printf("Comando HELP seleccionado.\n");
            //consoleExit();
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

//    while (shouldExecute) {
	while (1) {

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



//
//void consolePrintMenu() {
//    // impresion por pantalla de los comandos disponibles
//    printf("\n\n");
//    printf(" STOP	  - Pausa la planificacion\n");
//    printf(" PLAY     - Continua planificacion en pausa\n");
//    printf(" BLOCK    - Bloquea el proceso ESI hasta ser desbloqueado\n");
//    printf(" UNBLOCK  - Desbloquea el proceso ESI bloqueado\n");
//    printf(" LIST     - Lista los procesos encolados esperando recurso\n");
//    printf(" KILL     - Finaliza proceso\n");
//    printf(" STATUS   - Consulta informacion de instancias del sistema\n");
//    printf(" DEADLOCK - Crea​r​ copia​ de​ bloque​ de​ archivo​\n");
//    printf(" CLEAR    - Limpiar pantalla\n");
//    printf(" MAN      - sintaxis de cada comando\n");
//    printf(" EXIT     - Salir de la consola\n");
//    printf("\n\n");
//}
