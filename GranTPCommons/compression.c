/*
 * compression.c
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#include "compression.h"

uint16_t setOffset(uint16_t* offset, uint16_t* offsetNow, uint16_t size) {
	*offsetNow = *offset;
	*offset += size;
	return size;
}

uint16_t setOffsetInt(uint16_t* offset, uint16_t* offsetNow) {
	return setOffset(offset, offsetNow, sizeof(int));
}

uint16_t setOffsetString(uint16_t* offset, uint16_t* offsetNow, int size) {
	return setOffset(offset, offsetNow, size * sizeof(char));
}

uint16_t setOffsetLong(uint16_t* offset, uint16_t* offsetNow) {
	return setOffset(offset, offsetNow, sizeof(long));
}

char* copyStringFromBuffer(char **origin) {
	int length;
	char *aux;
	memcpy(&length, *origin, sizeof(int));
	(*origin) += sizeof(int);
	aux = (char*) malloc(length + 1);
	memcpy(aux, *origin, length);
	(*origin) += length;
	aux[length] = '\0';
	return aux;
}

char* copyStringToBuffer(char **buffer, char* origin) {
	int length = strlen(origin);
	memcpy(*buffer, &length, sizeof(int));
	(*buffer) += sizeof(int);
	memcpy(*buffer, origin, length);
	(*buffer) += length;
	return (*buffer);
}

int copyIntFromBuffer(char** buffer) {
	int aux;
	memcpy(&aux, *buffer, sizeof(int));
	(*buffer) += sizeof(int);
	return aux;
}
char* copyIntToBuffer(char** buffer, int value) {
	memcpy(*buffer, &value, sizeof(int));
	(*buffer) += sizeof(int);
	return (*buffer);
}

long copyLongFromBuffer(char** buffer) {
	long aux;
	memcpy(&aux, *buffer, sizeof(long));
	(*buffer) += sizeof(long);
	return aux;
}

char* copyLongToBuffer(char** buffer, long value) {
	memcpy(*buffer, &value, sizeof(long));
	(*buffer) += sizeof(long);
	return (*buffer);
}

char *copySizeFromBuffer(char**buffer, int size) {
	char* b = (char*) malloc(size);
	memcpy(b, *buffer, size);
	(*buffer) += size;
	return b;
}

char *copySizeToBuffer(char**buffer, char*data, int size) {
	memcpy(*buffer, data, size);
	(*buffer) += size;
	return (*buffer);
}

