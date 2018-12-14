/*
 * funcionesSAFA.c
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */

#include "funcionesSAFA.h"
#include <grantp/structCommons.h>
#include <grantp/configuracion.h>
#include "planificadorCorto.h"
#include "planificadorLargo.h"

//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA EL MANEJO DEL SELECT
//------------------------------------------------------------------------------------------------------------------

void addNewSocketToMaster(int socket) {
    pthread_mutex_lock(&mutexMaster);
    FD_SET(socket, &master);
    log_trace_mutex(logger, "Se agrega el socket %d a la lista de sockets", socket);
    updateMaxfd(socket);
    pthread_mutex_unlock(&mutexMaster);
}

int getMaxfd() {
    int m;
    pthread_mutex_lock(&mutexMaxfd);
    m = maxfd;
    pthread_mutex_unlock(&mutexMaxfd);
    return m;
}

void updateMaxfd(int socket) {
    if (socket > getMaxfd()) {
        pthread_mutex_lock(&mutexMaxfd);
        maxfd = socket;
        pthread_mutex_unlock(&mutexMaxfd);
    }
}

void deleteSocketFromMaster(int socket) {
    pthread_mutex_lock(&mutexMaster);
    FD_CLR(socket, &master);
    log_trace_mutex(logger, "Se saca el socket %d de la lista de sockets", socket);
    pthread_mutex_unlock(&mutexMaster);
}

void updateReadset() {
    pthread_mutex_lock(&mutexMaster);
    pthread_mutex_lock(&mutexReadset);
    readset = master;
    pthread_mutex_unlock(&mutexReadset);
    pthread_mutex_unlock(&mutexMaster);
}

int isSetted(int socket) {
    int s;
    pthread_mutex_lock(&mutexReadset);
    s = FD_ISSET(socket, &readset);
    pthread_mutex_unlock(&mutexReadset);
    return s;
}


//======================================================================================================================================
//============================================FUNCIONES Consola=========================================================================
//======================================================================================================================================

void setPlay() {
    pthread_mutex_lock(&mutexConsole);
    console = 1;
    pthread_mutex_unlock(&mutexConsole);
}

void setPause() {
    pthread_mutex_lock(&mutexConsole);
    console = 0;
    pthread_mutex_unlock(&mutexConsole);
}

int getConsoleStatus() {
    int aux = 0;
    pthread_mutex_lock(&mutexConsole);
    aux = console;
    pthread_mutex_unlock(&mutexConsole);
    return aux;
}

void playExecute() {
    pthread_mutex_lock(&mutexConsole);
    scheduler = 1;
    pthread_mutex_unlock(&mutexConsole);
}

void stopExecute() {
    pthread_mutex_lock(&mutexConsole);
    scheduler = 0;
    pthread_mutex_unlock(&mutexConsole);
}

int getExecute() {
    int aux = 0;
    pthread_mutex_lock(&mutexConsole);
    aux = scheduler;
    pthread_mutex_unlock(&mutexConsole);
    return aux;
}



//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES GDT y DTB
//------------------------------------------------------------------------------------------------------------------

//TODO: HACER LOS FREE PARA CUANDO SE TENGAN QUE LIBTERAR LOS DTBS!!!!!

t_dtb *crearNuevoDTB(char *dirScript) {

	sumarGDTCounter();
	int aux = obtenerGDTCounter();
	int size = 8*sizeof(int) + sizeof(t_list) + strlen(dirScript) + 1;
    t_dtb *newDTB = malloc(size);
	newDTB->idGDT = aux;
	newDTB->dirEscriptorio = malloc((strlen(dirScript) + 1));
	memcpy(newDTB->dirEscriptorio, dirScript, (strlen(dirScript) + 1));
//	newDTB->dirEscriptorio = dirScript;
	newDTB->programCounter = 0;
	newDTB->flagInicializado = 1;
	newDTB->tablaDirecciones = list_create();
	newDTB->cantidadLineas = 0;
	newDTB->realizOpDummy = 0;
	newDTB->quantumRestante = 0;
	newDTB->cantIO = 0;
	newDTB->esDummy = false;
	return newDTB;
}

int agregarDTBaNEW(t_dtb *dtb) {
    pthread_mutex_lock(&mutexNewList);
    int i = colaNew->elements_count;
    if (list_add(colaNew, dtb) != i) {
        log_info_mutex(logger, "no se pudo agregar el elemento a la lista");
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock(&mutexNewList);
    return EXIT_SUCCESS;
}

void sumarGDTCounter() {
    pthread_mutex_lock(&mutexgdtCounter);
    gdtCounter++;
    pthread_mutex_unlock(&mutexgdtCounter);
}

int obtenerGDTCounter() {
    int aux = 0;
    pthread_mutex_lock(&mutexgdtCounter);
    aux = gdtCounter;
    pthread_mutex_unlock(&mutexgdtCounter);
    return aux;
}

t_cpus *crearCpu() {
	//Se inicializa vacio, despues el PLP lo rellena cuando le pide al PCP
    t_cpus *cpu = malloc(sizeof(t_cpus));
	cpu->socket = 0;
	cpu->libre = 0;//0 = libre, 1= en uso
	return cpu;
}

int bloquearDTB(t_dtb * dtb){
	//logica para bloquear dtb
	pthread_mutex_lock(&mutexEjecutandoList);
	int i = buscarDTBEnCola(colaEjecutando,dtb);
	if(i<0){
		log_error_mutex(logger, "Error al bloquear el dtb: %d", dtb->idGDT);
		return EXIT_FAILURE;
	}
	t_dtb * dtbABloquear = list_remove(colaEjecutando,i);
	pthread_mutex_unlock(&mutexEjecutandoList);
	//actualizo el quantum restante
	dtbABloquear->quantumRestante = dtb->quantumRestante;
	dtbABloquear->programCounter = dtb->programCounter;
	pthread_mutex_lock(&mutexBloqueadosList);
	list_add(colaBloqueados,dtbABloquear);
	pthread_mutex_unlock(&mutexBloqueadosList);
	return EXIT_SUCCESS;
}

int desbloquearDTB(t_dtb * dtb){
	//logica para desbloquear dtb
	pthread_mutex_lock(&mutexBloqueadosList);
	int i = buscarDTBEnCola(colaBloqueados,dtb);
	pthread_mutex_unlock(&mutexBloqueadosList);
	if(i<0){
		return EXIT_FAILURE;
	}
	pthread_mutex_lock(&mutexBloqueadosList);
	t_dtb * dtbADesbloquear = list_remove(colaBloqueados,i);
	pthread_mutex_unlock(&mutexBloqueadosList);
	pthread_mutex_lock(&mutexReadyList);
	list_add(colaReady,dtbADesbloquear);
	pthread_mutex_unlock(&mutexReadyList);

	return EXIT_SUCCESS;
}

int abortarDTB(t_dtb * dtb, int socketCPU){
	//logica para abortar dtb

	//primero busco en la cola de ejecutando
	int result = pasarDTBdeEXECaFINALIZADO(dtb);

	//si no estaba ejecutando lo busco en la lista de bloqueados
	if(result == EXIT_FAILURE){
		result = pasarDTBdeBLOQUEADOaFINALIZADO(dtb);
	}else{
		log_error_mutex(logger, "SE ABORTÓ EL PROCESO ID: %d", dtb->idGDT);
		//si estaba ejecutando -> Se hace signal del semaforo y se libera la cpu
		sem_post(&semaforoCpu);
		liberarCpu(socketCPU);
	}
	return result;
}

int abortarDTBNuevo(t_dtb * dtb){
	//logica para abortar dtb

	//primero busco en la cola de ejecutando
	int result = pasarDTBdeNEWaEXIT(dtb);

	return result;
}

int finEjecucionPorQuantum(t_dtb * dtb){
	return pasarDTBdeEXECaREADY(dtb);
}

int confirmacionDMA(int pid, int result){
	if(result == EXIT_SUCCESS){ //SE CARGÓ CORRECTAMENTE
		//desbloqueo el proceso segun el algoritmo
		result = desbloquearDTBsegunAlgoritmo(pid);
	}else{ //HUBO UN ERROR AL CARGAR EL PROCESO EN MEMORIA
		//se finaliza el proceso
		pthread_mutex_lock(&mutexBloqueadosList);
		t_dtb * dtb = buscarDTBPorPIDenCola(colaBloqueados, pid);
		pthread_mutex_unlock(&mutexBloqueadosList);
		pasarDTBdeBLOQUEADOaFINALIZADO(dtb);
	}
	return result;
}
int buscarDTBEnCola(t_list * cola, t_dtb * dtbABuscar){
	int index = -1;
	int listSize = list_size(cola);
	if(listSize<= 0) return index;

	for(int i = 0; i<listSize;i++){
		t_dtb * dtb = list_get(cola,i);
		if(dtb->idGDT == dtbABuscar->idGDT){
			index = i;
			break;
		}
	}
	return index;
}
t_dtb * buscarDTBPorPIDenCola(t_list * cola, int pid){
	t_dtb * dtb;
	int listSize = list_size(cola);
	if(listSize<= 0) return NULL;

	for(int i = 0; i<listSize;i++){
		dtb = list_get(cola,i);
		if(dtb->idGDT == pid){
			return dtb;
			break;
		}
	}
	return dtb;
}

int buscarPosicionPorPIDenCola(t_list * cola, int pid){
	t_dtb * dtb;
	int listSize = list_size(cola);
	if(listSize<= 0) return -1;

	for(int i = 0; i<listSize;i++){
		dtb = list_get(cola,i);
		if(dtb->idGDT == pid){
			return i;
			break;
		}
	}
	return -1;
}

int desbloquearDTBsegunAlgoritmo(int pid){
	//desbloqueo el proceso dependiendo del algoritmo indicado
	pthread_mutex_lock(&mutexBloqueadosList);
	t_dtb * dtb = buscarDTBPorPIDenCola(colaBloqueados,pid);
	pthread_mutex_unlock(&mutexBloqueadosList);
	if(dtb->idGDT > 0){
		switch(conf->algoritmo){
		case RR:
			pasarDTBdeBLOQaREADY(dtb);
			break;
		case VRR:
			pasarDTBSegunQuantumRestante(dtb);
			break;
		}
	}else{
		//el proceso no está bloqueado
		log_error_mutex(logger, "Error al desbloquear el proceso pid: %d. No se encontró en la lista de bloqueados", pid);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void actualizarIODtb(t_dtb * dtb, int cantIo, int cantLineasProceso){

	pthread_mutex_lock(&mutexNewList);
	int index = buscarDTBEnCola(colaNew,dtb);
	if(index<0){
		log_error_mutex(logger, "No se encontró el dtb en la cola de new para actualizar cantidad de IO y lineas");
	}else{
		t_dtb * dtbAModificar = list_remove(colaNew,index);
		dtbAModificar->cantIO = cantIo;
		dtbAModificar->cantidadLineas = cantLineasProceso;
		list_add_in_index(colaNew,index,dtbAModificar);
	}

	pthread_mutex_unlock(&mutexNewList);
}

void actualizarTablaDirecciones(int pid, char * path){
	pthread_mutex_lock(&mutexBloqueadosList);
	// TODO: OJO QUE ESTÁ DEVOLVIENDO EL DUMMY Y NO EL GDT BUSCADO
	int index = buscarPosicionPorPIDenCola(colaBloqueados,pid);
	if(index >= 0){
		t_dtb * dtbAModificar = list_get(colaBloqueados,index);
		list_add(dtbAModificar->tablaDirecciones,path);
		list_add_in_index(colaBloqueados,index,dtbAModificar);
		log_info_mutex(logger, "Se actualizó la tabla de direcciones del proceso: %d", pid);
	}else{
		log_error_mutex(logger,"NO se encontró el proceso en la cola de bloqueados");
	}
	pthread_mutex_unlock(&mutexBloqueadosList);
}

void liberarCpu(int socketCpu){
	for(int i = 0; i<list_size(listaCpus);i++){
		t_cpus * cpu = list_get(listaCpus,i);
		if(cpu->socket == socketCpu){
			cpu->libre = 0;
		}
	}
}

//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA MANEJO DEL DUMMY
//------------------------------------------------------------------------------------------------------------------

/*
 * DTB PARA EL DUMMY
 * Se inicializa en bloqueado
 */
t_dtb *crearDummyDTB() {

	//Se inicializa vacio, despues el PLP lo rellena cuando le pide al PCP
	dummyDTB = malloc(sizeof(t_dtb));
	dummyDTB->idGDT = 0;
	dummyDTB->dirEscriptorio = NULL;
	dummyDTB->programCounter = 0;
	dummyDTB->flagInicializado = false;
	dummyDTB->tablaDirecciones = NULL;
	dummyDTB->realizOpDummy = 0;
	dummyDTB->cantidadLineas = 0;
	dummyDTB->cantIO = 0;
	dummyDTB->esDummy = true;
	dummyDTB->quantumRestante = 0;
    pthread_mutex_lock(&mutexBloqueadosList);
	list_add(colaBloqueados,dummyDTB);
	pthread_mutex_unlock(&mutexBloqueadosList);
	return dummyDTB;
}


void desbloquearDummy(){
    pthread_mutex_lock(&mutexDummy);
    dummyBloqueado = 0;
    pthread_mutex_unlock(&mutexDummy);
    //desbloquear dummy
    pthread_mutex_lock(&mutexBloqueadosList);
    t_dtb * dummy = list_find(colaBloqueados,(void *)obtenerDummy);
    pthread_mutex_unlock(&mutexBloqueadosList);
    desbloquearDTB(dummy);
}

void bloquearDummy(){
    pthread_mutex_lock(&mutexDummy);
    dummyBloqueado = 1;
    pthread_mutex_unlock(&mutexDummy);
    //bloquear dummy
    pthread_mutex_lock(&mutexEjecutandoList);
    t_dtb * dummy = list_find(colaEjecutando,(void *)obtenerDummy);
    dummy->idGDT = 0;
    dummy->dirEscriptorio = "\0";
    pthread_mutex_unlock(&mutexEjecutandoList);
    bloquearDTB(dummy);
}


/*
 * Funcion para setear el path del scriptorio en el dummy
 * params: (char *) path a cargar
 */
void setearPathEnDummy(char * path){
	pthread_mutex_lock(&mutexDummy);
	dummyDTB->dirEscriptorio = path;
	pthread_mutex_unlock(&mutexDummy);
}

/*
 * Funcion para setear el flag de inicializacion del dummy
 * Params:(int) 1 o 0
 */
void setearFlagInicializacionDummy(int num){
	pthread_mutex_lock(&mutexDummy);
	dummyDTB->flagInicializado = num;
	pthread_mutex_unlock(&mutexDummy);
}
int obtenerEstadoDummy() {
    int aux = 0;
    pthread_mutex_lock(&mutexDummy);
    aux = dummyBloqueado;
    pthread_mutex_unlock(&mutexDummy);
    return aux;
}

int obtenerFlagDummy() {
    int aux = 0;
    pthread_mutex_lock(&mutexDummy);
    aux = dummyDTB->flagInicializado;
    pthread_mutex_unlock(&mutexDummy);
    return aux;
}


//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA LAS METRICAS
//------------------------------------------------------------------------------------------------------------------

void actualizarTotalSentenciasEjecutadas(int cantidadDeSentencias){
	//mutexTotalSentencias
	//totalSentenciasEjecutadas
    pthread_mutex_lock(&mutexTotalSentencias);
    totalSentenciasEjecutadas = totalSentenciasEjecutadas + cantidadDeSentencias;
    pthread_mutex_unlock(&mutexTotalSentencias);
}

void actualizarSentenciasPasaronPorDAM(int cantidadDeSentencias){
    pthread_mutex_lock(&mutexSentenciasXDAM);
    sentenciasXDAM = sentenciasXDAM + cantidadDeSentencias;
    pthread_mutex_unlock(&mutexSentenciasXDAM);
}


