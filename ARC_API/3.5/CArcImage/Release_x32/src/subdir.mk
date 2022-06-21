################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ArcImageCAPI.cpp \
../src/CArcImage.cpp \
../src/DllMain.cpp 

OBJS += \
./src/ArcImageCAPI.o \
./src/CArcImage.o \
./src/DllMain.o 

CPP_DEPS += \
./src/ArcImageCAPI.d \
./src/CArcImage.d \
./src/DllMain.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -fpic -std=c++0x -m32 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


