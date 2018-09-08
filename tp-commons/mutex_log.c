/*
 * mutex_log.c
 *
 *  Created on: 21/10/2017
 *      Author: Martin Gauna
 */

#include "mutex_log.h"

#define log_impl_template_mutex(log_function, level_enum) 								\
		void log_function(t_log_mutex* logger, const char* message_template, ...) { 	\
			char* nuevo;																\
			va_list arguments;															\
			va_start(arguments, message_template);										\
			nuevo = string_from_vformat(message_template, arguments);					\
			va_end(arguments);															\
			pthread_mutex_lock(logger->mutex);											\
			switch(level_enum){															\
			case LOG_LEVEL_TRACE:														\
				log_trace(logger->logger, nuevo);										\
			break;																		\
			case LOG_LEVEL_DEBUG:														\
				log_debug(logger->logger, nuevo);										\
			break;																		\
			case LOG_LEVEL_INFO:														\
				log_info(logger->logger, nuevo);										\
			break;																		\
			case LOG_LEVEL_WARNING:														\
				log_warning(logger->logger, nuevo);										\
			break;																		\
			case LOG_LEVEL_ERROR:														\
				log_error(logger->logger, nuevo);										\
			break;																		\
			}																			\
			pthread_mutex_unlock(logger->mutex);										\
			free(nuevo);																\
		}\

/**
*	Funcion que inicializa el log y el mutex - el resto de las funciones se usan de forma similar a las de la catedra.
*/
t_log_mutex* log_create_mutex(char* file, char* program_name, bool is_active_console, t_log_level level){
	t_log_mutex*  logger = (t_log_mutex*) malloc(sizeof(t_log_mutex));
	logger->mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(logger->mutex, NULL);
	logger->logger = log_create(file, program_name, is_active_console, level);
	return logger;
}

log_impl_template_mutex(log_trace_mutex, LOG_LEVEL_TRACE);
log_impl_template_mutex(log_debug_mutex, LOG_LEVEL_DEBUG);
log_impl_template_mutex(log_info_mutex, LOG_LEVEL_INFO);
log_impl_template_mutex(log_warning_mutex, LOG_LEVEL_WARNING);
log_impl_template_mutex(log_error_mutex, LOG_LEVEL_ERROR);

//void log_info_mutex(t_log_mutex* logger,const char* mensaje, ...) {
//	char* nuevo;
//	va_list arguments;
//	va_start(arguments, mensaje);
//	nuevo = string_from_vformat(mensaje, arguments);
//	va_end(arguments);
//
//	pthread_mutex_lock(logger->mutex);
//	log_info(logger->logger, nuevo);
//	pthread_mutex_unlock(logger->mutex);
//	free(nuevo);
//}
//
//void log_warning_mutex(t_log_mutex* logger,const char* mensaje, ...) {
//	char* nuevo;
//	va_list arguments;
//	va_start(arguments, mensaje);
//	nuevo = string_from_vformat(mensaje, arguments);
//	va_end(arguments);
//
//	pthread_mutex_lock(logger->mutex);
//	log_warning(logger->logger, nuevo);
//	pthread_mutex_unlock(logger->mutex);
//	free(nuevo);
//}
//
//void log_error_mutex(t_log_mutex* logger,const char* mensaje, ...) {
//	char* nuevo;
//	va_list arguments;
//	va_start(arguments, mensaje);
//	nuevo = string_from_vformat(mensaje, arguments);
//	va_end(arguments);
//
//	pthread_mutex_lock(logger->mutex);
//	log_error(logger->logger, nuevo);
//	pthread_mutex_unlock(logger->mutex);
//	free(nuevo);
//}
//
//void log_debug_mutex(t_log_mutex* logger,const char* mensaje, ...) {
//	char* nuevo;
//	va_list arguments;
//	va_start(arguments, mensaje);
//	nuevo = string_from_vformat(mensaje, arguments);
//	va_end(arguments);
//
//	pthread_mutex_lock(logger->mutex);
//	log_debug(logger->logger, nuevo);
//	pthread_mutex_unlock(logger->mutex);
//	free(nuevo);
//}
//
//void log_trace_mutex(t_log_mutex* logger,const char* mensaje, ...) {
//	char* nuevo;
//	va_list arguments;
//	va_start(arguments, mensaje);
//	nuevo = string_from_vformat(mensaje, arguments);
//	va_end(arguments);
//
//	pthread_mutex_lock(logger->mutex);
//	log_debug(logger->logger, nuevo);
//	pthread_mutex_unlock(logger->mutex);
//	free(nuevo);
//
//}

void log_destroy_mutex(t_log_mutex* logger) {
	pthread_mutex_lock(logger->mutex);
	log_destroy(logger->logger);
	pthread_mutex_unlock(logger->mutex);
	pthread_mutex_destroy(logger->mutex);
	free(logger->mutex);
	free(logger);
}

void log_lock(t_log_mutex* logger){
	pthread_mutex_lock(logger->mutex);
}

void log_unlock(t_log_mutex* logger){
	pthread_mutex_unlock(logger->mutex);
}
