################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Libraries/TinyGPSPlus/TinyGPS++.cpp 

OBJS += \
./Libraries/TinyGPSPlus/TinyGPS++.o 

CPP_DEPS += \
./Libraries/TinyGPSPlus/TinyGPS++.d 


# Each subdirectory must supply rules for building sources it contributes
Libraries/TinyGPSPlus/%.o Libraries/TinyGPSPlus/%.su Libraries/TinyGPSPlus/%.cyclo: ../Libraries/TinyGPSPlus/%.cpp Libraries/TinyGPSPlus/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WLE5xx -c -I../Core/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../Drivers/CMSIS/Include -I"D:/.Embedded/.GITHUB/guillermo-loradesign-fw/Firmware-Development/STM32CubeIDE/TX-Board/Libraries/TinyGPSPlus" -I../SubGHz_Phy/App -I../SubGHz_Phy/Target -I../Utilities/trace/adv_trace -I../Utilities/misc -I../Utilities/sequencer -I../Utilities/timer -I../Utilities/lpm/tiny_lpm -I../Middlewares/Third_Party/SubGHz_Phy -I../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver -I"D:/.Embedded/.GITHUB/guillermo-loradesign-fw/Firmware-Development/STM32CubeIDE/TX-Board/Application" -I"D:/.Embedded/.GITHUB/guillermo-loradesign-fw/Firmware-Development/STM32CubeIDE/TX-Board/Components/GPS" -I"D:/.Embedded/.GITHUB/guillermo-loradesign-fw/Firmware-Development/STM32CubeIDE/TX-Board/Components/MCP7940" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Libraries-2f-TinyGPSPlus

clean-Libraries-2f-TinyGPSPlus:
	-$(RM) ./Libraries/TinyGPSPlus/TinyGPS++.cyclo ./Libraries/TinyGPSPlus/TinyGPS++.d ./Libraries/TinyGPSPlus/TinyGPS++.o ./Libraries/TinyGPSPlus/TinyGPS++.su

.PHONY: clean-Libraries-2f-TinyGPSPlus

