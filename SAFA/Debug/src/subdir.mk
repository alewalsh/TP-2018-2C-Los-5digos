################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/SAFA.c \
../src/consola.c \
../src/funcionesConsola.c \
../src/funcionesSAFA.c \
../src/handlerConexiones.c 

OBJS += \
./src/SAFA.o \
./src/consola.o \
./src/funcionesConsola.o \
./src/funcionesSAFA.o \
./src/handlerConexiones.o 

C_DEPS += \
./src/SAFA.d \
./src/consola.d \
./src/funcionesConsola.d \
./src/funcionesSAFA.d \
./src/handlerConexiones.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/Git/tp-2018-2c-Los-5digos/GranTPCommons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


