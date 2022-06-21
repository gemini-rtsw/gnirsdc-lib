################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ArcTiffFileJAPI.cpp \
../src/DllMain.cpp 

OBJS += \
./src/ArcTiffFileJAPI.o \
./src/DllMain.o 

CPP_DEPS += \
./src/ArcTiffFileJAPI.d \
./src/DllMain.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/home/streit/Documents/ARC_API/3.5/CArcTiffFile/src -I/home/streit/Documents/ARC_API/3.5/JavaJDK/x32/include/linux -I/home/streit/Documents/ARC_API/3.5/JavaJDK/x32/include -I/home/streit/Documents/ARC_API/3.5/tiff-4.0.3/linux/x32/include -O3 -Wall -c -fmessage-length=0 -fpic -std=c++0x -m32 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


