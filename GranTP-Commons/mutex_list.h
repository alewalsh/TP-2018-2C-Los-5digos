/*
 * mutex_list.h
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#ifndef MUTEXLIST_H_
#define MUTEXLIST_H_

#include <commons/collections/list.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct{
	pthread_mutex_t* mutex;
	t_list* lista;
}t_mutex_list;



void agregar_elem(t_list* lista,pthread_mutex_t mutex, void* elem);
void quitar_elem(t_list* lista,pthread_mutex_t mutex, int elem, void* contenedor);
t_mutex_list * mutex_list_create();
void* mutex_list_get(t_mutex_list* mList, int index);
int mutex_list_add(t_mutex_list *, void *element);
void mutex_list_destroy(t_mutex_list *list);
void* mutex_list_remove(t_mutex_list *mList, int index);
void* mutex_list_remove_by_condition(t_mutex_list *mList, bool(*condition)(void*));


#endif /* COMANDOS_H_ */
