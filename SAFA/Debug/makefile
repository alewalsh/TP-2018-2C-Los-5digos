################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: SAFA

dependents:
	-cd /home/utnso/Git/tp-2018-2c-Los-5digos/GranTPCommons/Debug && $(MAKE) clean all

# Tool invocations
SAFA: $(OBJS) $(USER_OBJS) /home/utnso/Git/tp-2018-2c-Los-5digos/GranTPCommons/Debug/libGranTPCommons.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/Git/tp-2018-2c-Los-5digos/GranTPCommons/Debug" -o "SAFA" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) SAFA
	-@echo ' '

.PHONY: all clean dependents
/home/utnso/Git/tp-2018-2c-Los-5digos/GranTPCommons/Debug/libGranTPCommons.so:

-include ../makefile.targets
