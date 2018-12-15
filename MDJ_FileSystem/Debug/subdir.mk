################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FileSystem.c \
../funcionesConsola.c \
../funcionesDAM.c 

OBJS += \
./FileSystem.o \
./funcionesConsola.o \
./funcionesDAM.o 

C_DEPS += \
./FileSystem.d \
./funcionesConsola.d \
./funcionesDAM.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2018-2c-Los-5digos/GranTPCommons" -IGranTPCommons -Icommons -Ireadline -Ipthread -Icrypto -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


