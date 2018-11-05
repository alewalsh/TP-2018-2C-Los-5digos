/*
 * configuracion.c
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#include "configuracion.h"
#include "structCommons.h"

int checkAmountOfParams(t_config *configFile, int amount, t_log *logger);

uint16_t getAlgorithm(char *algorithm) {
    if (string_equals_ignore_case(algorithm, "RR")) {
        return RR;
    } else if (string_equals_ignore_case(algorithm, "VRR")) {
        return VRR;
    } else if (string_equals_ignore_case(algorithm, "PROPIO")) {
        return PROPIO;
    } else
    	return 0;
}

uint16_t getModoEjecucion(char * modo)
{
	if (string_equals_ignore_case(modo, "SEG")) {
		return SEG;
	} else if (string_equals_ignore_case(modo, "TPI")) {
		return TPI;
	} else if (string_equals_ignore_case(modo, "SPA")){
		return SPA;
	} else
		return 0;
}

void *cargarConfiguracion(char *path, processType configType, t_log *logger) {
    t_config *configFile;
    configSAFA *safa;
    configCPU *cpu;
    configDAM *dam;
    configFM9 *funesMemory;
    configMDJ *mdj;
    void *ret;

    configFile = config_create(path);
    if (!configFile) {
        log_error(logger, "No se encontro el archivo de configuracion.\n");
        return NULL;
    } else if (configFile->properties->elements_amount == 0) {
        config_destroy(configFile);
        log_error(logger, "Archivo de configuracion vacio. Cargarlo con los elementos "
                          "correspondientes\n");
        return NULL;
    }

    switch (configType) {
        case SAFA:
            if (checkAmountOfParams(configFile, 5, logger)) {
                ret = NULL;
                break;
            }
            safa = (configSAFA *) malloc(sizeof(configSAFA));
            safa->puerto = leerPuerto(configFile, "PUERTO", logger);
            char *algor = leerString(configFile, "ALGORITMO", logger);
            safa->algoritmo = getAlgorithm(algor);
            free(algor);
            safa->quantum = leerInt(configFile, "QUANTUM", logger);
            safa->grado_mp = leerInt(configFile, "MULTIPROGRAMACION", logger);
            safa->retardo = leerInt(configFile, "RETARDO", logger);
            ret = safa;
            break;
        case CPU:
            if (checkAmountOfParams(configFile, 7, logger)) {
                ret = NULL;
                break;
            }
            cpu = (configCPU *) malloc(sizeof(configCPU));
            cpu->ipSAFA = leerIP(configFile, "IP_SAFA", logger);
            cpu->puertoSAFA = leerPuerto(configFile, "PUERTO_SAFA", logger);
            cpu->ipDAM = leerIP(configFile, "IP_DAM", logger);
            cpu->puertoDAM = leerPuerto(configFile, "PUERTO_DAM", logger);
            cpu->ipFM9 = leerIP(configFile, "IP_FM9", logger);
            cpu->puertoFM9 = leerPuerto(configFile, "PUERTO_FM9", logger);
            cpu->retardo = leerInt(configFile, "RETARDO", logger);
            ret = cpu;
            break;
        case DAM:
            if (checkAmountOfParams(configFile, 8, logger)) {
                ret = NULL;
                break;
            }
            dam = (configDAM *) malloc(sizeof(configDAM));
            dam->puertoDAM = leerPuerto(configFile, "PUERTO", logger);
            dam->ipSAFA = leerIP(configFile, "IP_SAFA", logger);
            dam->puertoSAFA = leerPuerto(configFile, "PUERTO_SAFA", logger);
            dam->ipMDJ = leerIP(configFile, "IP_MDJ", logger);
            dam->puertoMDJ = leerPuerto(configFile, "PUERTO_MDJ", logger);
            dam->ipFM9 = leerIP(configFile, "IP_FM9", logger);
            dam->puertoFM9 = leerPuerto(configFile, "PUERTO_FM9", logger);
            dam->transferSize = leerInt(configFile, "TRANSFER_SIZE", logger);
            ret = dam;
            break;
        case MDJ:
            if (checkAmountOfParams(configFile, 3, logger)) {
                ret = NULL;
                break;
            }
            mdj = (configMDJ *) malloc(sizeof(configMDJ));
            mdj->puertoMDJ = leerPuerto(configFile, "PUERTO", logger);
            mdj->ip_propia = leerIP(configFile, "IP_PROPIA", logger);
            mdj->puntoMontaje = leerString(configFile, "PUNTO_MONTAJE", logger);
            mdj->retardo = leerInt(configFile, "RETARDO", logger);
            mdj->tam_bloq = leerInt(configFile, "TAMANIO_BLOQUES", logger);
            mdj->cant_bloq = leerInt(configFile, "CANTIDAD_BLOQUES", logger);
            mdj->magic_num = leerString(configFile, "MAGIC_NUMBER", logger);
            ret = mdj;
            break;
        case FM9:
        	if (checkAmountOfParams(configFile, 6, logger)) {
				ret = NULL;
				break;
			}
            funesMemory = (configFM9 *) malloc(sizeof(configFM9));
            funesMemory->puertoFM9 = leerPuerto(configFile, "PUERTO", logger);
            funesMemory->ip_propia = leerIP(configFile, "IP_PROPIA", logger);
            char *modo = leerString(configFile, "MODO", logger);
            funesMemory->modoEjecucion = getModoEjecucion(modo);
            free(modo);
            funesMemory->tamMemoria = leerInt(configFile, "TAMANIO", logger);
            funesMemory->tamMaxLinea = leerInt(configFile, "MAX_LINEA", logger);
            funesMemory->tamPagina = leerInt(configFile, "TAM_PAGINA", logger);
            ret = funesMemory;
        	break;
        default:
            ret = NULL;
    }
    config_destroy(configFile);
    return ret;
}

int checkAmountOfParams(t_config *configFile, int amount, t_log *logger) {
    if (config_keys_amount(configFile) < amount) {
        log_error(logger, "No se encuentran inicializados todos los parametros de "
                          "configuracion requeridos.");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

char *leerString(t_config *configFile, char *parametro, t_log *logger) {
    char *string, *aux;
    if (config_has_property(configFile, parametro)) {
        aux = config_get_string_value(configFile, parametro);
        string = string_duplicate(aux);
    } else {
        log_error(logger, "No se encuentra el parametro en el archivo de config.");
        exit(EXIT_FAILURE);
    }
    log_trace(logger, "%s:%s", parametro, string);
    return string;
}

int leerInt(t_config *configFile, char *parametro, t_log *logger) {
    int valor;
    if (config_has_property(configFile, parametro)) {
        valor = config_get_int_value(configFile, parametro);
    } else {
        log_error(logger, "No se encuentra el parametro en el archivo de configuracion.");
        exit(EXIT_FAILURE);
    }
    log_trace(logger, "%s:%d", parametro, valor);
    return valor;
}

int leerPuerto(t_config *configFile, char *parametro, t_log *logger) {
    int puerto = 0;
    puerto = leerInt(configFile, parametro, logger);
    validar_puerto(puerto, logger);
    return puerto;
}

char *leerIP(t_config *configFile, char *parametro, t_log *logger) {
    char *ip = "";
    ip = leerString(configFile, parametro, logger);
    return ip;
}

long leerLong(t_config *configFile, char *parametro, t_log *logger) {
    long valor = 0;
    if (config_has_property(configFile, parametro)) {
        valor = config_get_int_value(configFile, parametro);
    } else {
        log_error(logger, "No se encuentra el parametro en el archivo de configuracion.");
        exit(EXIT_FAILURE);
    }
    return valor;
}

void validar_puerto(int puerto, t_log *logger) {
    if (puerto < PUERTO_MAX) {
        log_error(logger, "El numero de puerto indicado se encuentra reservado para el sistema.");
        exit(EXIT_FAILURE);
    }
    return;
}

void freeConfig(void *conf, processType processType) {
	configSAFA *safa;
	configCPU *cpu;
	configDAM *dam;
	configFM9 *funesMemory;
	configMDJ *mdj;


	//TODO: Verificar porque hay que hacer mas free de los que esta, faltan algunas cosas

    if (conf != NULL) {
        switch (processType) {
            case SAFA:
            	safa = (configSAFA *) conf;
            	//TODO: Juan revisara las cosas que se tienen que liberar enSAFA(ver planif)
                free(safa);
                break;
            case CPU:
                cpu = (configCPU *) conf;
                free(cpu->ipDAM);
                free(cpu->ipSAFA);
                free(cpu->ipFM9);
                free(cpu);
                break;
            case DAM:
            	dam = (configDAM *) conf;
                free(dam->ipFM9);
                free(dam->ipMDJ);
                free(dam->ipSAFA);
                free(dam);
                break;
            case FM9:
            	funesMemory = (configFM9 *) conf;
            	free(funesMemory->ip_propia);
                free(funesMemory);
                break;
            case MDJ:
            	mdj = (configMDJ *) conf;
            	free(mdj->ip_propia);
            	free(mdj->puntoMontaje);
            	free(mdj->magic_num);
            	free(mdj);
            	break;
            default:
                break;
        }
    }
}

t_list *leerArray(t_config *configFile, char *parametro, t_log *logger) {
    t_list *lista = list_create();

    char *p = leerString(configFile, parametro, logger);

    //Si no tienen claves bloqueadas agregar "-", esto lo compruebo aca
    if (strlen(p) == 1) {
        return lista;
    }

    char **params = string_split(p, ",");

    for (int i = 0; params[i] != NULL; i++) {
        printf("\n %s \n", params[i]);
        list_add(lista, params[i]);
    }

    //free(params);
    return lista;
}

