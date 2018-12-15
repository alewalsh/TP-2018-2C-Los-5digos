################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../grantp/comandos.c \
../grantp/compression.c \
../grantp/configuracion.c \
../grantp/fileFunctions.c \
../grantp/javaStrings.c \
../grantp/mutex_list.c \
../grantp/mutex_log.c \
../grantp/parser.c \
../grantp/socket.c \
../grantp/split.c \
../grantp/structCommons.c 

OBJS += \
./grantp/comandos.o \
./grantp/compression.o \
./grantp/configuracion.o \
./grantp/fileFunctions.o \
./grantp/javaStrings.o \
./grantp/mutex_list.o \
./grantp/mutex_log.o \
./grantp/parser.o \
./grantp/socket.o \
./grantp/split.o \
./grantp/structCommons.o 

C_DEPS += \
./grantp/comandos.d \
./grantp/compression.d \
./grantp/configuracion.d \
./grantp/fileFunctions.d \
./grantp/javaStrings.d \
./grantp/mutex_list.d \
./grantp/mutex_log.d \
./grantp/parser.d \
./grantp/socket.d \
./grantp/split.d \
./grantp/structCommons.d 


# Each subdirectory must supply rules for building sources it contributes
grantp/%.o: ../grantp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


