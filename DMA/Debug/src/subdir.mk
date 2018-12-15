################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/DMA.c \
../src/funcionesDMA.c 

OBJS += \
./src/DMA.o \
./src/funcionesDMA.o 

C_DEPS += \
./src/DMA.d \
./src/funcionesDMA.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/Git/tp-2018-2c-Los-5digos/GranTPCommons" -Ipthread -Icommons -IGranTPCommons -Ireadline -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


