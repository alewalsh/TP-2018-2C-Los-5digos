#include "FileSystem.h"


//atender peticion de manera concurrente?? al mismo tiempo.
//todo semaforos para todos.

//todo AGREGAR RETARDO

//todo crear log aparte para ver en consola aparte.

//ver como hago con archivos ya creados. en obtener y guardar datos.


void consoleExit() {
	setExit();
}

void responderDAM() {
//void responderDAM(t_package pkg, int socketFD) {

	printf(" Llega de alguna manera a responder DAM \n");

	int opcion;
	opcion = 2; //sera asignado por escuchar al DAM

	switch(opcion) {
//	switch(pkg.code) {
		case 1: //validar archivo ++ [Path]
			printf("validar archivo\n");

			//TODO ver como condigo el filename. con path incluido. o estan todos si o si dentro de /archivos?
			//en pkg.data ???

			char *path = string_new();

			string_append(&path,"/home/utnso/mountTP/FIFA_FS/Metadata/bitmap.bin");
//			string_append(&path,"/home/utnso/mountTP/FIFA_FS/Metadata/jalala.txt");

		    int status;
		    status = validarArchivo(path);
			if(status){
				printf("Archivo OK\n");
			}else{
				printf("Archivo inexistente \n");
			}

		break;

		case 2: //crear archivo ++ [Path, Cantidad de Líneas]

			printf("crear archivo \n");

			int cantidad_lineas;
			cantidad_lineas = 13; //parametro de la funcion. ver como obtengo.

			int bytePorLinea = 5; //lo sabe el fm9, ver como lo consigo.

			int pesoTotalLineas = cantidad_lineas * bytePorLinea;

			int cantidadTotalBloques = pesoTotalLineas / configuracion->tam_bloq;

			if(pesoTotalLineas % configuracion->tam_bloq != 0){
				cantidadTotalBloques++;
			}

			int lineasPorbloque = configuracion->tam_bloq / bytePorLinea;

			//checkea en bitarray si hay la cantdad de bloques necesario.
			if(cantidadTotalBloques <= cuantosBitsLibres()){

				//crea path
				//TODO ver como me lo pasa. relativo a /archivos? abosluto?

				//crea archivo
				char *pathArchivoNuevo = string_new();
				string_append(&pathArchivoNuevo,configuracion->puntoMontaje);
				string_append(&pathArchivoNuevo,"Archivos/prueba.bin");

				FILE *archivoNuevo;
				archivoNuevo = fopen(pathArchivoNuevo, "ab");

				//busca bloques disponibles
				int bloques[cantidadTotalBloques];

				int posicion;
				posicion = 0;

				int ocupados;
				ocupados = 0;

				while(ocupados < cantidadTotalBloques){
					if(bitarray_test_bit(bitarray, posicion) == 0){
						bitarray_set_bit(bitarray, posicion);
						bloques[ocupados] = posicion;
						ocupados ++;
					}
					posicion ++;
				}

				//paso los nuevos bloques tomados al bitarray fisico.
				actualizarBitmapHDD();

				char *metadataNewArchivo = string_new();

				//Poner maximo y listo en metadat archivos tamanio.

				string_append(&metadataNewArchivo,"TAMANIO_ARCHIVO=");
				string_append(&metadataNewArchivo,string_itoa(pesoTotalLineas));
				string_append(&metadataNewArchivo,"\n");

				string_append(&metadataNewArchivo,"BLOQUES_ARCHIVOS=[");
				//if pesoTotalLineas != 0 ?
				string_append(&metadataNewArchivo,string_itoa(bloques[0]));

				int posicionBloques;
				posicionBloques = 1;

				while(posicionBloques <= (ocupados-1)){
					string_append(&metadataNewArchivo,",");
					string_append(&metadataNewArchivo,string_itoa(bloques[posicionBloques]));
					posicionBloques ++;
				}

				string_append(&metadataNewArchivo,"]");
				char *metadataParaFwrite = string_from_format(metadataNewArchivo);
				fwrite(metadataParaFwrite , 1 , strlen(metadataParaFwrite) , archivoNuevo );

				fclose(archivoNuevo);
				free(metadataParaFwrite);
				free(metadataNewArchivo);

				//Creo o abro bloques de datos e inserto cantidad de lineas vacias dentro.
				int bloquesCreados;
				bloquesCreados = 0;

				int lineasInsertadas;
				lineasInsertadas = 1; //porque ya se crean con una linea por default.

				while(bloquesCreados < (ocupados - 1)){
					char *pathBloqueI = string_new();
					string_append(&pathBloqueI,configuracion->puntoMontaje);
					string_append(&pathBloqueI,"Bloques/");
					string_append(&pathBloqueI,string_itoa(bloques[bloquesCreados]));
					string_append(&pathBloqueI,".bin");

					FILE *bloqueI;
					bloqueI = fopen(pathBloqueI, "wb");

					while(lineasInsertadas < lineasPorbloque){
						fwrite("\n", 1, strlen("\n"), bloqueI);
						lineasInsertadas++;
					}

					lineasInsertadas = 1;

					bloquesCreados ++;
					fclose(bloqueI);
					free(pathBloqueI);
				}
				//en el ultimo bloque fijarse si lo lleno o le meto menos.
				int lineasUltimoAarchivo;

				if(pesoTotalLineas % configuracion->tam_bloq != 0){
					int bytesAAgregar = pesoTotalLineas % configuracion->tam_bloq;

					lineasUltimoAarchivo = bytesAAgregar / bytePorLinea;

					if(bytesAAgregar % bytePorLinea != 0){
						lineasUltimoAarchivo++;
					}

				}else{
					lineasUltimoAarchivo = lineasPorbloque;
				}

				char *pathBloqueI = string_new();
				string_append(&pathBloqueI,configuracion->puntoMontaje);
				string_append(&pathBloqueI,"Bloques/");
				string_append(&pathBloqueI,string_itoa(bloques[bloquesCreados]));
				string_append(&pathBloqueI,".bin");

				FILE *bloqueI;
				bloqueI = fopen(pathBloqueI, "wb");

				while(lineasInsertadas < lineasUltimoAarchivo){
					fwrite("\n", 1, strlen("\n"), bloqueI);
					lineasInsertadas++;
				}

				bloquesCreados ++;
				fclose(bloqueI);
				free(pathBloqueI);

			}else{
				//todo error, no hay suficientes bloques libres para lamacenar el archivo. ERROR 50002
			}

		break;

		//TODO path invalido devuelve error.

		case 3: //obtener datos ++ [Path, Offset, Size]
			printf("obtener datos \n");

			//devuelve los bytes. get

		break;

		case 4: //guardar datos ++ [Path, Offset, Size, Buffer]
			printf("guardar datos \n");
			//para inicial no tenr en cuenta que se pasa de cantidad de bloques ya asgiandos.
			//todo en la escritura controlo todo el tiempo si no se paso del tamanio y ahi sigo al siguiente que le corresponde al archivo?
			//todo si se pasa del tamanio del archico me corresponde tirar error o no pasara? no, lo puedo hacer crecer.

			//si me madnan gurdar, gurado todo excpeto falta de blques.

		break;

		case 5: //borrar archivo ++ [Path]
			printf("borrar archivo \n");
		break;
	}
}


int validarArchivo(char *path){
	struct stat buffer;
	int status;
	status = stat(path,&buffer);
	if(status < 0){
		return 0;
	}else{
		return 1;
	}
}

int getExit() {
	return shouldExit;
}

void setExit() {
	pthread_mutex_lock(&mutexExit);
	shouldExit = 1;
	pthread_mutex_unlock(&mutexExit);
}
