/*
 ============================================================================
 Name        : SAFA.c
 Author      : Alejandro Walsh
 Version     : 1.0
 Copyright   :
 Description : Módulo SAFA para el TP de SO 2c 2018
 ============================================================================
 */

#include "SAFA.h"
#include <grantp/configuracion.h>

pthread_attr_t tattr;


int main(int argc, char ** argv) {

	//Creo las Variables locales
    pthread_t threadConsola;
    pthread_t threadConexiones;
    pthread_t threadCambioConfig;
    pthread_t threadCortoPlazo;
    pthread_t threadLargoPlazo;

    /*inicializacion de recursos y carga de configuracion
     /home/utnso/git/tp-2018-2c-Los-5digos/SAFA/config"*/
    if (argc > 0)
    	inicializarRecursos(argv[1]);
    else
    {
    	log_error_mutex(logger, "Faltan parametros para inicializar el SAFA.");
    	return EXIT_FAILURE;
	}

    printf("Recursos incializados \n");

    rutaConfig = argv[1];

    rutaConfigSinCofig = string_substring_until(rutaConfig, strlen(rutaConfig) - strlen("/config.cfg"));

	printf("Se crearan los hilos de safa \n");

	//Inicializo la consola del Planificador y los threads correspondientes
    pthread_create(&threadConexiones, &tattr, (void *) manejarConexiones, NULL);
    pthread_create(&threadCambioConfig, &tattr, (void *) cambiosConfig, NULL);
    pthread_create(&threadCortoPlazo, &tattr, (void *) manejoCortoPlazo, NULL);
    pthread_create(&threadLargoPlazo, &tattr, (void *) manejoLargoPlazo, NULL);
    pthread_create(&threadConsola, &tattr, (void *) mainConsola, NULL);

    printf("Hilos creados \n");

    /*pthread_join(threadConexiones,NULL);
    pthread_join(threadCambioConfig,NULL);
    pthread_join(threadCortoPlazo,NULL);
    pthread_join(threadLargoPlazo,NULL);*/
    pthread_join(threadConsola,NULL);

    liberarRecursos();
    destruirListas();
	return EXIT_SUCCESS;
}


void inicializarRecursos(char * pathConfig){

    logger = log_create_mutex("SAFA.log", "SAFA", 0, LOG_LEVEL_INFO);
    if (strcmp(pathConfig,"")==0)
    {
        log_error_mutex(logger, "No existe el archivo indicado");
        exit(1);
    }
    //Cargo el archivo configuracion
    conf = (configSAFA *) cargarConfiguracion(pathConfig, SAFA, logger->logger);

    if(conf==NULL){
        log_error_mutex(logger, "No existe archivo configuracion");
        exit(1);
    }

    log_info_mutex(logger, "Puerto de Escucha: %d", conf->puerto);
    log_info_mutex(logger, "Algoritmo de Planificacion leido de configuracion: %d", conf->algoritmo);

    pthread_attr_init(&tattr);
    //pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);

    initList();
    initMutexs();
    FD_ZERO(&master);
    FD_ZERO(&readset);
}


void liberarRecursos(){

    pthread_mutex_destroy(&mutexNewList);
    pthread_mutex_destroy(&mutexReadyList);
    pthread_mutex_destroy(&mutexBloqueadosList);
    pthread_mutex_destroy(&mutexEjecutandoList);
    pthread_mutex_destroy(&mutexExitList);
    pthread_mutex_destroy(&mutexReadyEspList);
    pthread_mutex_destroy(&mutexMaster);
    pthread_mutex_destroy(&mutexReadset);
    pthread_mutex_destroy(&mutexMaxfd);
    pthread_mutex_destroy(&mutexExit);
    pthread_mutex_destroy(&mutexStop);
    pthread_mutex_destroy(&mutexConsole);
    pthread_mutex_destroy(&mutexgdtCounter);
    pthread_mutex_destroy(&mutexDummy);
    pthread_mutex_destroy(&semDummy);
    pthread_mutex_destroy(&semCargadoEnMemoria);
    pthread_mutex_destroy(&mutexPlanificando);

    sem_destroy(&semaforpGradoMultiprgramacion);
    sem_destroy(&mandadosPorConsola);
    sem_destroy(&desbloquearDTBDummy);
    sem_destroy(&enviarDtbACPU);

    log_destroy_mutex(logger);
    freeConfig(conf, SAFA);
    free(rutaConfigSinCofig);
}

void destruirListas(){
	list_destroy_and_destroy_elements(colaNew,(void*)destruir_dtb);
	list_destroy_and_destroy_elements(colaReady,(void*)destruir_dtb);
	list_destroy_and_destroy_elements(colaReadyEspecial,(void*)destruir_dtb);
	list_destroy_and_destroy_elements(colaBloqueados,(void*)destruir_dtb);
	list_destroy_and_destroy_elements(colaEjecutando,(void*)destruir_dtb);
	list_destroy_and_destroy_elements(colaExit,(void*)destruir_dtb);
	list_destroy_and_destroy_elements(listaMetricasLP,(void *) destruirMetricaLP);
}

void destruir_dtb(t_dtb * dtb){
	free(dtb->dirEscriptorio);
	list_destroy_and_destroy_elements(dtb->tablaDirecciones,(void *)liberarDirecciones);
	free(dtb);
}

void liberarDirecciones(char * direccion){
	free(direccion);
}

void destruirMetricaLP(t_metricaLP * metrica){
	free(metrica);
}
void initMutexs(){

	//Se inicializan todos los semaforos MUTEX a ser utilizados
	pthread_mutex_init(&mutexMaster, NULL);
	pthread_mutex_init(&mutexReadset, NULL);
	pthread_mutex_init(&mutexMaxfd, NULL);

	//Mutex para las listas
	pthread_mutex_init(&mutexNewList, NULL);
	pthread_mutex_init(&mutexReadyList, NULL);
	pthread_mutex_init(&mutexBloqueadosList, NULL);
	pthread_mutex_init(&mutexEjecutandoList, NULL);
	pthread_mutex_init(&mutexExitList, NULL);
	pthread_mutex_init(&mutexReadyEspList, NULL);

	pthread_mutex_init(&mutexExit, NULL);
	pthread_mutex_init(&mutexStop, NULL);
	pthread_mutex_init(&mutexConsole, NULL);
	pthread_mutex_init(&mutexgdtCounter, NULL);
	pthread_mutex_init(&mutexDummy, NULL);
	pthread_mutex_init(&semCargadoEnMemoria, NULL);

	pthread_mutex_init(&semDummy, NULL);
	sem_init(&semaforpGradoMultiprgramacion, 0 ,conf->grado_mp);
	sem_init(&mandadosPorConsola, 0, 0);

	sem_init(&desbloquearDTBDummy,0,0);
	sem_init(&enviarDtbACPU,0,0);
}


void initList() {
	colaNew = list_create();
	colaReady = list_create();
	colaReadyEspecial = list_create();
	colaBloqueados = list_create();
	colaEjecutando = list_create();
	colaExit = list_create();
	listaMetricasLP = list_create();
}

void cambiosConfig(){
       char buffer[BUF_LEN];

       // Al inicializar inotify este nos devuelve un descriptor de archivo
       int file_descriptor = inotify_init();
       if (file_descriptor < 0) {
               perror("inotify_init");
       }

       // Creamos un monitor sobre un path indicando que eventos queremos escuchar
       int watch_descriptor = inotify_add_watch(file_descriptor, rutaConfigSinCofig , IN_CLOSE_WRITE );

       // El file descriptor creado por inotify, es el que recibe la información sobre los eventos ocurridos
       // para leer esta información el descriptor se lee como si fuera un archivo comun y corriente pero
       // la diferencia esta en que lo que leemos no es el contenido de un archivo sino la información
       // referente a los eventos ocurridos
       int length = read(file_descriptor, buffer, BUF_LEN);
       if (length < 0) {
               perror("read");
       }

       int offset = 0;

       // Luego del read buffer es un array de n posiciones donde cada posición contiene
       // un eventos ( inotify_event ) junto con el nombre de este.
       while (offset < length) {

               // El buffer es de tipo array de char, o array de bytes. Esto es porque como los
               // nombres pueden tener nombres mas cortos que 24 caracteres el tamaño va a ser menor
               // a sizeof( struct inotify_event ) + 24.
               struct inotify_event *event = (struct inotify_event *) &buffer[offset];
                       // El campo "len" nos indica la longitud del tamaño del nombre
               if (event->len) {
                       // Dentro de "mask" tenemos el evento que ocurrio y sobre donde ocurrio
                       // sea un archivo o un directorio
                       if (event->mask & IN_CLOSE_WRITE ) {
                               if (event->mask & IN_ISDIR) {
                                       printf("opcion 1 -- %s\n", event->name);
                               } else {
                                       //para filtarar solo los cambios en el config y no otros archivos.clea
//                                     if(strcmp(event->name, "config.cfg") ==0){
                                               int ALGORITMOviejo = conf->algoritmo;
                                               int QUANTUMviejo = conf->quantum;
                                               int MULTIPROGRAMACIONviejo = conf->grado_mp;
                                               int RETARDOviejo = conf->retardo;

                                               freeConfig(conf, SAFA);
                                               conf = (configSAFA *) cargarConfiguracion(rutaConfig, SAFA, logger->logger);

                                               if(ALGORITMOviejo != conf->algoritmo){
                                                       printf("Se modifico el algortimo de %d a %d \n", ALGORITMOviejo, conf->algoritmo);
                                               }
                                               if(QUANTUMviejo != conf->quantum){
                                                       printf("Se modifico el quentum de %d a %d \n", QUANTUMviejo, conf->quantum);
                                                       //todo enviar a todas las cpus.
                                               }
                                               if(MULTIPROGRAMACIONviejo != conf->grado_mp){
                                                       printf("Se modifico el grado de MP de %d a %d \n", MULTIPROGRAMACIONviejo, conf->grado_mp);
                                                       //modoficar el semaforo semaforpGradoMultiprgramacion
                                               }
                                               if(RETARDOviejo != conf->retardo){
                                                       printf("Se modifico el retardo de %d a %d \n", RETARDOviejo, conf->retardo);
                                                       //todo ver para que se usa.
                                               }
//                                     }
                               }
                       }
               }
               offset += sizeof (struct inotify_event) + event->len;
       }
               inotify_rm_watch(file_descriptor, watch_descriptor);
       close(file_descriptor);
       cambiosConfig();
}

void manejoLargoPlazo() {
	planificadorLP();
}

void manejoCortoPlazo() {
	planificadorCP();
}


