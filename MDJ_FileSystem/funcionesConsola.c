#include "funcionesConsola.h"

void consoleExit() {
    setExit();
}

// ----------------------------------------------------------------------------------------------------------------------
//  FUNCIONES QUE USA EL THREAD
// ----------------------------------------------------------------------------------------------------------------------

int getExit() {
    return shouldExit;
}

void setExit() {
    pthread_mutex_lock(&mutexExit);
    shouldExit = 1;
    pthread_mutex_unlock(&mutexExit);
}
