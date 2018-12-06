/*
 * split.c
 *
 *  Created on: 15 nov. 2018
 *      Author: utnso
 */


#include "split.h"
#include "stddef.h"

/**
 * @NAME: str_split
 * @DESC: Se encarga de dividir un char * con un demilitador
 * @PARAMS: {char *} 	string a dividir
 * 			{const char}	demilitador
 * 			{int} Int a cargar con la cantidad de lineas formadas.
 * @RETURN: {char **} Punteros de los punteros de las lineas
 */
char** str_split(char* a_str, const char a_delim, int * count)
{
    char** result    = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    (*count) = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    int i = 0;
    /* Count how many elements will be extracted. */
    while(a_str[i] != '\0')
	{
		if(a_str[i] == '\n')
		{
			(*count)++;
			last_comma = tmp;
		}
		i++;
	}

    /* Add space for trailing token. */
    (*count) += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    (*count)++;

    result = malloc(sizeof(char*) * (*count));

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        *(result + idx) = 0;
    }

    return result;
}
