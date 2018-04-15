################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/plugins/plc/MyPlc.cpp 

OBJS += \
./src/plugins/plc/MyPlc.o 

CPP_DEPS += \
./src/plugins/plc/MyPlc.d 


# Each subdirectory must supply rules for building sources it contributes
src/plugins/plc/%.o: ../src/plugins/plc/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


