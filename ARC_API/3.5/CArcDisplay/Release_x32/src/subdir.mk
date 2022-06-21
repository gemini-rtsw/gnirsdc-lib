################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ArcDisplayCAPI.cpp \
../src/CArcDisplay.cpp \
../src/DllMain.cpp 

OBJS += \
./src/ArcDisplayCAPI.o \
./src/CArcDisplay.o \
./src/DllMain.o 

CPP_DEPS += \
./src/ArcDisplayCAPI.d \
./src/CArcDisplay.d \
./src/DllMain.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/home/streit/Documents/ARC_API/3.5/xpa-2.1.14/linux/x32/include -O3 -Wall -c -fmessage-length=0 -fPIC -m32 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


