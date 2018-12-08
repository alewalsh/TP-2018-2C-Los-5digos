/*
 * parser.c
 *
 *  Created on: 8 oct. 2018
 *      Author: Alejandro Walsh
 */

#include "parser.h"

#define RETURN_ERROR t_cpu_operacion ERROR={ .valido = false }; return ERROR
#define ABRIR_KEYWORD "abrir"
#define ASIGNAR_KEYWORD "asignar"
#define CONCENTRAR_KEYWORD "concentrar"
#define WAIT_KEYWORD "wait"
#define SIGNAL_KEYWORD "signal"
#define FLUSH_KEYWORD "flush"
#define CLOSE_KEYWORD "close"
#define CREAR_KEYWORD "crear"
#define BORRAR_KEYWORD "borrar"

void destruir_operacion(t_cpu_operacion op){
	if(op._raw){
		string_iterate_lines(op._raw, (void*) free);
		free(op._raw);
	}
}


t_list * parseoInstrucciones(char * path, int cantidadLineas, t_log_mutex * logger )
{
	FILE * fp = fopen(path, "r");
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	if (fp == NULL){
		log_error_mutex(logger, "Error al abrir el archivo: ");
	}
	t_list * listaInstrucciones = list_create();
	int i = 1;
	//TODO: no hay ningun archivo que explicitamente contenga todas las instrucciones, podria ser un char ** e ir utilizando las posiciones. Esto esta bien?
	while ((read = getline(&line, &len, fp)) != -1)
	{
		bool ultimaLinea = (i == cantidadLineas);
		t_cpu_operacion parsed = parse(line, ultimaLinea);
		if(parsed.valido)
		{
			if(!parsed.esComentario)
				list_add(listaInstrucciones, &parsed);

			destruir_operacion(parsed);
//			// TODO: PRUEBA TEMPORAL PARA VERIFICAR QUE NO DESTRUYE LA REFERENCIA QUE SE AGREGÓ EN LISTA INSTRUCCIONES
//			t_cpu_operacion * operacion = list_get(listaInstrucciones, 0);
//			printf("Accion: %d", operacion->keyword);
//			printf("Argumento 1: %s", operacion->argumentos.ABRIR.path);
		}
		else
		{
			log_error_mutex(logger, "La linea <%d> no es valida\n", i);
		}
		i++;
	}

	fclose(fp);
	if (line)
		free(line);

	return listaInstrucciones;
}

t_cpu_operacion parse(char* line, bool ultimaLinea){

	t_cpu_operacion ret = {
		.valido = true
	};
	bool lineaVacia = string_equals_ignore_case(line, "");
	//TODO: Revisar por que no entra en el segundo if, hay algun error?
	if (ultimaLinea && !lineaVacia)
	{
		fprintf(stderr, "El archivo debe finalizar con una línea vacía\n");
		RETURN_ERROR;
	}
	if(!ultimaLinea && lineaVacia)
	{
		fprintf(stderr, "No pude interpretar una linea vacia\n");
		RETURN_ERROR;
	}
	char* auxLine = string_duplicate(line);
	string_trim(&auxLine);
	char** split = string_n_split(auxLine, 3, " ");

	char* keyword = split[0];
	char* clave = split[1];

	ret._raw = split;

	if(!ultimaLinea)
	{
		if (!string_starts_with(keyword,"#"))
		{
			if(clave == NULL && strcmp(keyword, CONCENTRAR_KEYWORD)!=0) {
				fprintf(stderr, "No habia parametros en la linea <%s>\n", (line));
				RETURN_ERROR;
			}
			if(strcmp(keyword, ABRIR_KEYWORD)==0){
				ret.keyword = ABRIR;
				ret.argumentos.ABRIR.path = split[1];
			}
			else if(strcmp(keyword, CONCENTRAR_KEYWORD)==0){
				ret.keyword = CONCENTRAR;
			}
			else if(strcmp(keyword, ASIGNAR_KEYWORD)==0){
				ret.keyword = ASIGNAR;
				ret.argumentos.ASIGNAR.path = split[1];
				ret.argumentos.ASIGNAR.linea = atoi(split[2]);
				ret.argumentos.ASIGNAR.datos = split[3];
			}
			else if(strcmp(keyword, WAIT_KEYWORD)==0){
				ret.keyword = WAIT;
				ret.argumentos.WAIT.recurso = split[1];
			}
			else if(strcmp(keyword, SIGNAL_KEYWORD)==0){
				ret.keyword = SIGNAL;
				ret.argumentos.SIGNAL.recurso = split[1];
			}
			else if(strcmp(keyword, FLUSH_KEYWORD)==0){
				ret.keyword = FLUSH;
				ret.argumentos.FLUSH.path = split[1];
			}
			else if(strcmp(keyword, CLOSE_KEYWORD)==0){
				ret.keyword = CLOSE;
				ret.argumentos.CLOSE.path = split[1];
			}
			else if(strcmp(keyword, CREAR_KEYWORD)==0){
				ret.keyword = CREAR;
				ret.argumentos.CREAR.path = split[1];
				ret.argumentos.CREAR.linea = atoi(split[2]);
			}
			else if(strcmp(keyword, BORRAR_KEYWORD)==0){
				ret.keyword = BORRAR;
				ret.argumentos.BORRAR.path = split[1];
			}
			else {
				fprintf(stderr, "No se encontro el keyword <%s>\n", keyword);\
				RETURN_ERROR;
			}
		}
		else
		{
			ret.esComentario = true;
		}
		free(auxLine);
	}
	return ret;
}
