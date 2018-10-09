/*
 * parser.h
 *
 *  Created on: 8 oct. 2018
 *      Author: utnso
 */

#ifndef PARSER_H_
#define PARSER_H_

	#include <stdlib.h>
	#include <stdio.h>
	#include <stdbool.h>
	#include <string.h>
	#include <commons/string.h>

	typedef struct {
		bool valido;
		enum {
			ABRIR,
			CONCENTRAR,
			ASIGNAR,
			WAIT,
			SIGNAL,
			FLUSH,
			CLOSE,
			CREAR,
			BORRAR
		} keyword;
		union {
			struct {
				char* path;
			} ABRIR;
			struct {
				char* path;
				int linea;
				char* datos;
			} ASIGNAR;
			struct {
				char* recurso;
			} WAIT;
			struct {
				char* recurso;
			} SIGNAL;
			struct {
				char* path;
			} FLUSH;
			struct {
				char* path;
			} CLOSE;
			struct {
				char* path;
				int linea;
			} CREAR;
			struct {
				char* path;
			} BORRAR;
		} argumentos;
		char** _raw; //Para uso de la liberación
	} t_cpu_operacion;

	/**
	* @NAME: parse
	* @DESC: interpreta una linea de un archivo ESI y
	*		 genera una estructura con el operador interpretado
	* @PARAMS:
	* 		line - Una linea de un archivo ESI
	* 		ultimaLinea - Indica que es la ultima linea del archivo
	*/
	t_cpu_operacion parse(char* line, bool ultimaLinea);

	/**
	* @NAME: destruir_operacion
	* @DESC: limpia la operacion generada por el parse
	* @PARAMS:
	* 		op - Una operacion obtenida del parse
	*/
	void destruir_operacion(t_cpu_operacion op);

#endif /* PARSER_H_ */
