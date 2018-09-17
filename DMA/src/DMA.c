/*
 ============================================================================
 Name        : DMA.c
 Author      : Franco Lopez
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "DMA.h"

int main(void) {
	configure_logger();
	cargarArchivoDeConfig();
	//inicializarDMA();

	exit_gracefully(0);
	return EXIT_SUCCESS;
}

void cargarArchivoDeConfig(){
	configDMA = cargarConfiguracion("config.cfg", DAM, logger);
	log_info(logger, "Archivo de configuraciones cargado correctamente");
}

void configure_logger(){
	logger = log_create("DAM.log", "DAM", true, LOG_LEVEL_INFO);
	log_info(logger, "Inicia proceso: Diego Armando Maradona (DAM)");
}

void inicializarDMA(){
	//Conectarse con S-AFA, MDJ y FM9
	//Crear hilo que mantenga la escucha con cada proceso
	int socketSafa = connect_to_server(configDMA->ipSAFA, configDMA->puertoSAFA, "S-AFA");
	int socketMdj = connect_to_server(configDMA->ipMDJ, configDMA->puertoMDJ, "MDJ");
	int socketFm = connect_to_server(configDMA->ipFM9, configDMA->puertoFM9, "FM9");
}

int connect_to_server(char * ip, char * port, char * servidor) {
  struct addrinfo hints;
  struct addrinfo *server_info;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;    // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
  hints.ai_socktype = SOCK_STREAM;  // Indica que usaremos el protocolo TCP

  getaddrinfo(ip, port, &hints, &server_info);  // Carga en server_info los datos de la conexion

  //Socket
  int server_socket = socket(server_info->ai_family, server_info->ai_socktype,server_info->ai_protocol);

  //Me conecto al server a traves del socket
  int retorno = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

  freeaddrinfo(server_info);  // No lo necesitamos mas
  // chequeo el retorno
  if(retorno < 0){
	  log_error(logger, "Error al conectar");
	  exit_gracefully(1);
  }

  log_info(logger, "Se conectó con exito a: (%c)", servidor);
  return server_socket;
}

//Funcion para cerrar el programa
void exit_gracefully(int return_nr) {
	log_info(logger,"Fin del proceso: DAM");

	log_destroy(logger);
	exit(return_nr);
}
