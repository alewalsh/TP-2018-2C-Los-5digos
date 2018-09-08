/*
 * mutex_list.c
 *
 *  Created on: 21/5/2017
 *      Author: utnso
 */

#include "mutex_list.h"

void agregar_elem(t_list* lista, pthread_mutex_t mutex, void* elem) {

	pthread_mutex_lock(&mutex);
	list_add(lista, elem);
	pthread_mutex_unlock(&mutex);

}

void quitar_elem(t_list* lista, pthread_mutex_t mutex, int elem, void* contenedor) {
	pthread_mutex_lock(&mutex);
	contenedor = list_get(lista,elem);
	pthread_mutex_unlock(&mutex);

}

t_mutex_list* mutex_list_create(){
	t_mutex_list * list = (t_mutex_list*) malloc(sizeof(t_mutex_list));
	list->mutex = (pthread_mutex_t*)  malloc(sizeof(pthread_mutex_t));
	list->lista = list_create();
	pthread_mutex_init(list->mutex, NULL);
	return list;
}

void* mutex_list_get(t_mutex_list* mList, int index){
	void* element;
	pthread_mutex_lock(mList->mutex);
	element = list_get(mList->lista, index);
	pthread_mutex_unlock(mList->mutex);
	return element;
}

int mutex_list_add(t_mutex_list *mList, void *element){
	int result;
	pthread_mutex_lock(mList->mutex);
	result = list_add(mList->lista, element);
	pthread_mutex_unlock(mList->mutex);
	return result;
}

void mutex_list_destroy(t_mutex_list *list){
	list_destroy(list->lista);
	pthread_mutex_destroy(list->mutex);
}

void* mutex_list_remove(t_mutex_list *mList, int index){
	void* elem;
	pthread_mutex_lock(mList->mutex);
	elem = list_remove(mList->lista, index);
	pthread_mutex_unlock(mList->mutex);
	return elem;
}

void* mutex_list_remove_by_condition(t_mutex_list *mList, bool(*condition)(void*)){
	void* elem;
	pthread_mutex_lock(mList->mutex);
	elem = list_remove_by_condition(mList->lista, condition);
	pthread_mutex_unlock(mList->mutex);
	return elem;
}

