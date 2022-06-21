################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ArcDeinterlaceCAPI.cpp \
../src/CArcDeinterlace.cpp \
../src/DllMain.cpp 

OBJS += \
./src/ArcDeinterlaceCAPI.o \
./src/CArcDeinterlace.o \
./src/DllMain.o 

CPP_DEPS += \
./src/ArcDeinterlaceCAPI.d \
./src/CArcDeinterlace.d \
./src/DllMain.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -fPIC -std=c++0x -m32 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


