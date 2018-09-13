################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../comandos.c \
../compression.c \
../configuracion.c \
../fileFunctions.c \
../javaStrings.c \
../mutex_list.c \
../mutex_log.c \
../socket.c \
../structCommons.c 

OBJS += \
./comandos.o \
./compression.o \
./configuracion.o \
./fileFunctions.o \
./javaStrings.o \
./mutex_list.o \
./mutex_log.o \
./socket.o \
./structCommons.o 

C_DEPS += \
./comandos.d \
./compression.d \
./configuracion.d \
./fileFunctions.d \
./javaStrings.d \
./mutex_list.d \
./mutex_log.d \
./socket.d \
./structCommons.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


