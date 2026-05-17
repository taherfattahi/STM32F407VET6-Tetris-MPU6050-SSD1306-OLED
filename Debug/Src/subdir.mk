################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/main.c \
../Src/mpu6050.c \
../Src/ssd1306.c \
../Src/syscalls.c \
../Src/sysmem.c \
../chip_headers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c \
../Src/tetris.c 

OBJS += \
./Src/main.o \
./Src/mpu6050.o \
./Src/ssd1306.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/system_stm32f4xx.o \
./Src/tetris.o 

C_DEPS += \
./Src/main.d \
./Src/mpu6050.d \
./Src/ssd1306.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/system_stm32f4xx.d \
./Src/tetris.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F4 -DSTM32F407VETx -DSTM32F407xx -c -I../Inc -I"C:/Users/taher/Desktop/stm32f405/my-work-example/STM32F407VET6-Tetris/chip_headers/CMSIS/Include" -I"C:/Users/taher/Desktop/stm32f405/my-work-example/STM32F407VET6-Tetris/chip_headers/CMSIS/Device/ST/STM32F4xx/Include" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/system_stm32f4xx.o: C:/Users/taher/Desktop/stm32f405/my-work-example/STM32F407VET6-Tetris/chip_headers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F4 -DSTM32F407VETx -DSTM32F407xx -c -I../Inc -I"C:/Users/taher/Desktop/stm32f405/my-work-example/STM32F407VET6-Tetris/chip_headers/CMSIS/Include" -I"C:/Users/taher/Desktop/stm32f405/my-work-example/STM32F407VET6-Tetris/chip_headers/CMSIS/Device/ST/STM32F4xx/Include" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/main.cyclo ./Src/main.d ./Src/main.o ./Src/main.su ./Src/mpu6050.cyclo ./Src/mpu6050.d ./Src/mpu6050.o ./Src/mpu6050.su ./Src/ssd1306.cyclo ./Src/ssd1306.d ./Src/ssd1306.o ./Src/ssd1306.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.cyclo ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su ./Src/system_stm32f4xx.cyclo ./Src/system_stm32f4xx.d ./Src/system_stm32f4xx.o ./Src/system_stm32f4xx.su ./Src/tetris.cyclo ./Src/tetris.d ./Src/tetris.o ./Src/tetris.su

.PHONY: clean-Src

