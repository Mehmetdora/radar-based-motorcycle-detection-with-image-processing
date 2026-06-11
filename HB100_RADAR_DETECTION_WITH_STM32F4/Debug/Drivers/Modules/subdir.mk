################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/Modules/ADC_DMA_Config.c \
../Drivers/Modules/FFT_DPS.c \
../Drivers/Modules/HELPER.c \
../Drivers/Modules/IIR_DSP.c \
../Drivers/Modules/RadarSignalHelper.c \
../Drivers/Modules/TimerDriver.c \
../Drivers/Modules/UARTDriver.c 

OBJS += \
./Drivers/Modules/ADC_DMA_Config.o \
./Drivers/Modules/FFT_DPS.o \
./Drivers/Modules/HELPER.o \
./Drivers/Modules/IIR_DSP.o \
./Drivers/Modules/RadarSignalHelper.o \
./Drivers/Modules/TimerDriver.o \
./Drivers/Modules/UARTDriver.o 

C_DEPS += \
./Drivers/Modules/ADC_DMA_Config.d \
./Drivers/Modules/FFT_DPS.d \
./Drivers/Modules/HELPER.d \
./Drivers/Modules/IIR_DSP.d \
./Drivers/Modules/RadarSignalHelper.d \
./Drivers/Modules/TimerDriver.d \
./Drivers/Modules/UARTDriver.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Modules/%.o Drivers/Modules/%.su Drivers/Modules/%.cyclo: ../Drivers/Modules/%.c Drivers/Modules/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I"/Users/mehmet_dora/Desktop/STM32/HB100_RADAR_DETECTION_WITH_STM32F4/Drivers/Modules" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/ARM/DSP/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-Modules

clean-Drivers-2f-Modules:
	-$(RM) ./Drivers/Modules/ADC_DMA_Config.cyclo ./Drivers/Modules/ADC_DMA_Config.d ./Drivers/Modules/ADC_DMA_Config.o ./Drivers/Modules/ADC_DMA_Config.su ./Drivers/Modules/FFT_DPS.cyclo ./Drivers/Modules/FFT_DPS.d ./Drivers/Modules/FFT_DPS.o ./Drivers/Modules/FFT_DPS.su ./Drivers/Modules/HELPER.cyclo ./Drivers/Modules/HELPER.d ./Drivers/Modules/HELPER.o ./Drivers/Modules/HELPER.su ./Drivers/Modules/IIR_DSP.cyclo ./Drivers/Modules/IIR_DSP.d ./Drivers/Modules/IIR_DSP.o ./Drivers/Modules/IIR_DSP.su ./Drivers/Modules/RadarSignalHelper.cyclo ./Drivers/Modules/RadarSignalHelper.d ./Drivers/Modules/RadarSignalHelper.o ./Drivers/Modules/RadarSignalHelper.su ./Drivers/Modules/TimerDriver.cyclo ./Drivers/Modules/TimerDriver.d ./Drivers/Modules/TimerDriver.o ./Drivers/Modules/TimerDriver.su ./Drivers/Modules/UARTDriver.cyclo ./Drivers/Modules/UARTDriver.d ./Drivers/Modules/UARTDriver.o ./Drivers/Modules/UARTDriver.su

.PHONY: clean-Drivers-2f-Modules

