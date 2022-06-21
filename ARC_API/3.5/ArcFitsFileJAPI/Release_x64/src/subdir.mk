################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ArcFitsFileJAPI.cpp \
../src/DllMain.cpp 

OBJS += \
./src/ArcFitsFileJAPI.o \
./src/DllMain.o 

CPP_DEPS += \
./src/ArcFitsFileJAPI.d \
./src/DllMain.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/home/streit/Documents/ARC_API/3.5/CArcFitsFile/src -I/home/streit/Documents/ARC_API/3.5/cfitsio-3330/linux/x64/include -I/home/streit/Documents/ARC_API/3.5/JavaJDK/x64/include -I/home/streit/Documents/ARC_API/3.5/JavaJDK/x64/include/linux -O3 -Wall -c -fmessage-length=0 -fPIC -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


