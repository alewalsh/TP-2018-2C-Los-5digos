################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/FunesMemory9.c \
../src/funcionesFM9.c 

OBJS += \
./src/FunesMemory9.o \
./src/funcionesFM9.o 

C_DEPS += \
./src/FunesMemory9.d \
./src/funcionesFM9.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/Git/tp-2018-2c-Los-5digos/GranTPCommons" -O0 -g3 -Wall -c -fmessage-length=0 -v -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


