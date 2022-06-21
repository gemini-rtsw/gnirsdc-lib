################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ArcTiffFileCAPI.cpp \
../src/CArcTiffFile.cpp \
../src/DllMain.cpp 

OBJS += \
./src/ArcTiffFileCAPI.o \
./src/CArcTiffFile.o \
./src/DllMain.o 

CPP_DEPS += \
./src/ArcTiffFileCAPI.d \
./src/CArcTiffFile.d \
./src/DllMain.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/home/streit/Documents/ARC_API/3.5/tiff-4.0.3/linux/x32/include -O3 -Wall -c -fmessage-length=0 -fPIC -std=c++0x -m32 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


