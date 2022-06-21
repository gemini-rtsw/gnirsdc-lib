################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ArcDeviceCAPI.cpp \
../src/ArcOSDefs.cpp \
../src/CArcDevice.cpp \
../src/CArcPCI.cpp \
../src/CArcPCIBase.cpp \
../src/CArcPCIe.cpp \
../src/CArcTools.cpp \
../src/CLog.cpp \
../src/CStringList.cpp \
../src/DllMain.cpp \
../src/TempCtrl.cpp 

OBJS += \
./src/ArcDeviceCAPI.o \
./src/ArcOSDefs.o \
./src/CArcDevice.o \
./src/CArcPCI.o \
./src/CArcPCIBase.o \
./src/CArcPCIe.o \
./src/CArcTools.o \
./src/CLog.o \
./src/CStringList.o \
./src/DllMain.o \
./src/TempCtrl.o 

CPP_DEPS += \
./src/ArcDeviceCAPI.d \
./src/ArcOSDefs.d \
./src/CArcDevice.d \
./src/CArcPCI.d \
./src/CArcPCIBase.d \
./src/CArcPCIe.d \
./src/CArcTools.d \
./src/CLog.d \
./src/CStringList.d \
./src/DllMain.d \
./src/TempCtrl.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -fpic -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


