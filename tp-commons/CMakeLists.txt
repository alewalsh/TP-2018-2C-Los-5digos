cmake_minimum_required(VERSION 3.10)
project(ProgramRangersCommons)

set(CMAKE_C_STANDARD 11)

add_library(ProgramRangersCommons SHARED comandos.c
        comandos.h
        compression.c
        compression.h
        configuracion.c
        configuracion.h
        fileFunctions.c
        fileFunctions.h
        javaStrings.c
        javaStrings.h
        mutex_list.c
        mutex_list.h
        mutex_log.c
        mutex_log.h
        socket.c
        socket.h
        structCommons.c
        structCommons.h)