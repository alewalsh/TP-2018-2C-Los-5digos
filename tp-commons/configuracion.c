/*
 * archivo_configuracion.c
 *
 *  Created on: 3/4/2017
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
    } else {
        return PROPIO;
    }
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
            char *algorithm = leerString(configFile, "ALG_PLAN", logger);
            safa->algoritmo = getAlgorithm(algorithm);
            free(algorithm);
            safa->estimacion = leerInt(configFile, "ESTIMACION_INICIAL", logger);
            int alfa = leerInt(configFile, "ALFA", logger);
            safa->alfa = alfa * 1.0 / 100;
            safa->ipCoord = leerIP(configFile, "IP_COOR", logger);
            safa->puertoCoord = leerPuerto(configFile, "PUERTO_COOR", logger);
            safa->claves_iniciales = leerString(configFile, "CLAVE_INICIAL_BLOQUEADA", logger);
            ret = safa;
            break;
        case CPU:
            if (checkAmountOfParams(configFile, 5, logger)) {
                ret = NULL;
                break;
            }
            coordinador = (configCoordinador *) malloc(sizeof(configCoordinador));
            coordinador->puerto = leerPuerto(configFile, "PUERTO", logger);
            coordinador->algoritmo = leerString(configFile, "ALG_DISTRIBUCION", logger);
            coordinador->cantEntradas = leerInt(configFile, "CANT_ENTRADAS", logger);
            coordinador->entradas = leerInt(configFile, "ENTRADA", logger);
            coordinador->retardo = leerInt(configFile, "RETARDO", logger);
            ret = coordinador;
            break;
        case DAM:
            if (checkAmountOfParams(configFile, 6, logger)) {
                ret = NULL;
                break;
            }
            esi = (configESI *) malloc(sizeof(configESI));
            esi->ipCoor = leerIP(configFile, "IP_COOR", logger);
            esi->puertoCoord = leerPuerto(configFile, "PUERTO_COOR", logger);
            esi->ipPlanificador = leerIP(configFile, "IP_PLANIFICADOR", logger);
            esi->puertoPlanificador = leerPuerto(configFile, "PUERTO_PLANIFICADOR", logger);
            esi->scriptPath = leerString(configFile, "SCRIPT_PATH", logger);
            ret = esi;
            break;
        case MDJ:
            if (checkAmountOfParams(configFile, 6, logger)) {
                ret = NULL;
                break;
            }
            instancia = (configInstancia *) malloc(sizeof(configInstancia));
            instancia->ipCoor = leerIP(configFile, "IP_COOR", logger);
            instancia->puertoCoord = leerPuerto(configFile, "PUERTO_COOR", logger);
            instancia->algoritmo = leerString(configFile, "ALG_REEMPLAZO", logger);
            instancia->path = leerString(configFile, "PATH", logger);
            instancia->nombre = leerString(configFile, "NOMBRE", logger);
            instancia->dump = leerInt(configFile, "DUMP", logger);
            ret = instancia;
            break;
        case FM9:
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
    configPlanificador *planificador;
    configCoordinador *coordinador;
    configInstancia *instancia;
    configESI *esi;

    if (conf != NULL) {
        switch (processType) {
            case PLANIFICADOR:
                planificador = (configPlanificador *) conf;
                free(planificador->ipCoord);
                /*chequear si tengo que hacer un free por cada elemento del array*/
                free(planificador->claves_iniciales);
                free(planificador);
                break;
            case COORDINADOR:
                coordinador = (configCoordinador *) conf;
                free(coordinador->algoritmo);
                free(coordinador);
                break;
            case ESI:
                esi = (configESI *) conf;
                free(esi->ipCoor);
                free(esi->ipPlanificador);
                free(esi->scriptPath);
                free(esi);
                break;
            case INSTANCIA:
                instancia = (configInstancia *) conf;
                free(instancia->ipCoor);
                free(instancia->algoritmo);
                free(instancia->path);
                free(instancia->nombre);
                free(instancia);
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
