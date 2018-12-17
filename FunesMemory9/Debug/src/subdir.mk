################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/FunesMemory9.c \
../src/TPI.c \
../src/commons.c \
../src/consolaFM9.c \
../src/funcionesFM9.c \
../src/segPaginada.c \
../src/segmentacionSimple.c 

OBJS += \
./src/FunesMemory9.o \
./src/TPI.o \
./src/commons.o \
./src/consolaFM9.o \
./src/funcionesFM9.o \
./src/segPaginada.o \
./src/segmentacionSimple.o 

C_DEPS += \
./src/FunesMemory9.d \
./src/TPI.d \
./src/commons.d \
./src/consolaFM9.d \
./src/funcionesFM9.d \
./src/segPaginada.d \
./src/segmentacionSimple.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2018-2c-Los-5digos/GranTPCommons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


