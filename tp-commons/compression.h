/*
 * compression.h
 *
 *  Created on: 19/9/2017
 *      Author: utnso
 */
#include <commons/log.h>
#include <commons/collections/list.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

uint16_t setOffset(uint16_t* offset,uint16_t* offsetNow,uint16_t size);
uint16_t setOffsetInt(uint16_t* offset,uint16_t* offsetNow);
uint16_t setOffsetString(uint16_t* offset,uint16_t* offsetNow, int size);
uint16_t setOffsetLong(uint16_t* offset,uint16_t* offsetNow);
char* copyStringFromBuffer(char **origin);
char* copyStringToBuffer(char **buffer, char* origin);
int copyIntFromBuffer (char** buffer);
char* copyIntToBuffer (char** buffer, int value);
long copyLongFromBuffer (char** buffer);
char* copyLongToBuffer (char** buffer, long value);
char *copySizeFromBuffer(char**buffer, int size);
char *copySizeToBuffer(char**buffer, char*data, int size);

