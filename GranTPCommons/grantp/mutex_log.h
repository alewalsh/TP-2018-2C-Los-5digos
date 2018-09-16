/*
 * mutex_log.h
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#ifndef MUTEXLOG_H_
#define MUTEXLOG_H_

#include <commons/string.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdarg.h>
//TODO Ver si no conviene cambiar la extern por un strcut, porque esto acopla mucho el uso de la lib.

extern pthread_mutex_t logMutex;


typedef struct {
	pthread_mutex_t* mutex;
	t_log* logger;
}t_log_mutex;



t_log_mutex* log_create_mutex(char* file, char* program_name, bool is_active_console, t_log_level level);
void log_info_mutex(t_log_mutex* logger,const char* mensaje, ...);
void log_warning_mutex(t_log_mutex* logger,const char* mensaje, ...);
void log_error_mutex(t_log_mutex* logger,const char* mensaje, ...);
void log_debug_mutex(t_log_mutex* logger,const char* mensaje, ...);
void log_trace_mutex(t_log_mutex* logger,const char* mensaje, ...);
void log_destroy_mutex(t_log_mutex* logger);
void log_lock(t_log_mutex* logger);
void log_unlock(t_log_mutex* logger);



#endif /* COMANDOS_H_ */
