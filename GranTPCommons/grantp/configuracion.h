/*
 * configuracion.h
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#ifndef CONFIGURACION_H_
#define CONFIGURACION_H_

#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <string.h>
#include <commons/config.h>
#include <stdint.h>

#define PUERTO_MAX 1024

typedef enum {
    SAFA, CPU, FM9, DAM, MDJ
} processType;

typedef struct {
    int puerto;
    uint16_t algoritmo;
    int quantum;
    int grado_mp;
    int retardo;
} configSAFA;

typedef struct {
    char *ipSAFA;
    int puertoSAFA;
    char *ipDAM;
    int puertoDAM;
    char *ipFM9;
    int puertoFM9;
    int retardo;
} configCPU;

typedef struct {
    int puertoDAM;
    char *ipSAFA;
    int puertoSAFA;
    char *ipMDJ;
    int puertoMDJ;
    char *ipFM9;
    int puertoFM9;
    int transferSize;
} configDAM;

typedef struct {
    int puertoFM9;
    char * ip_propia;
    uint16_t modoEjecucion;
    int tamMemoria;
    int tamMaxLinea;
    int tamPagina;
} configFM9;

typedef struct {
    int puertoMDJ;
    char *ip_propia;
    char *puntoMontaje;
    int retardo;
} configMDJ;

void *cargarConfiguracion(char *path, processType configType, t_log *logger);

char *leerString(t_config *configFile, char *parametro, t_log *logger);

int leerInt(t_config *configFile, char *parametro, t_log *logger);

long leerLong(t_config *configFile, char *parametro, t_log *logger);

int leerPuerto(t_config *configFile, char *parametro, t_log *logger);

char *leerIP(t_config *configFile, char *parametro, t_log *logger);

void cerrar_archivo_config();

void validar_ip(char *ip, t_log *logger);

void validar_puerto(int puerto, t_log *logger);

void freeConfig(void *conf, processType processType);

//t_list *leerArray(t_config *configFile, char *parametro, t_log *logger);

#endif /* COMANDOS_H_ */
