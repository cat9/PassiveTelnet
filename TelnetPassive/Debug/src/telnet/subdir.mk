################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/telnet/authenc.cc \
../src/telnet/commands.cc \
../src/telnet/environ.cc \
../src/telnet/genget.cc \
../src/telnet/main.cc \
../src/telnet/netlink.cc \
../src/telnet/network.cc \
../src/telnet/ring.cc \
../src/telnet/sys_bsd.cc \
../src/telnet/telnet.cc \
../src/telnet/terminal.cc \
../src/telnet/tn3270.cc \
../src/telnet/utilities.cc 

OBJS += \
./src/telnet/authenc.o \
./src/telnet/commands.o \
./src/telnet/environ.o \
./src/telnet/genget.o \
./src/telnet/main.o \
./src/telnet/netlink.o \
./src/telnet/network.o \
./src/telnet/ring.o \
./src/telnet/sys_bsd.o \
./src/telnet/telnet.o \
./src/telnet/terminal.o \
./src/telnet/tn3270.o \
./src/telnet/utilities.o 

CC_DEPS += \
./src/telnet/authenc.d \
./src/telnet/commands.d \
./src/telnet/environ.d \
./src/telnet/genget.d \
./src/telnet/main.d \
./src/telnet/netlink.d \
./src/telnet/network.d \
./src/telnet/ring.d \
./src/telnet/sys_bsd.d \
./src/telnet/telnet.d \
./src/telnet/terminal.d \
./src/telnet/tn3270.d \
./src/telnet/utilities.d 


# Each subdirectory must supply rules for building sources it contributes
src/telnet/%.o: ../src/telnet/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -DUSE_TERMIO -Incurses -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


