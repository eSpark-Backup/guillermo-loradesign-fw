################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Components/MCP7940/MCP7940.cpp 

OBJS += \
./Components/MCP7940/MCP7940.o 

CPP_DEPS += \
./Components/MCP7940/MCP7940.d 


# Each subdirectory must supply rules for building sources it contributes
Components/MCP7940/%.o Components/MCP7940/%.su Components/MCP7940/%.cyclo: ../Components/MCP7940/%.cpp Components/MCP7940/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WLE5xx -c -I../Core/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../Drivers/CMSIS/Include -I"D:/GitHub/guillermo-loradesign-fw/Firmware-Development/STM32CubeIDE/TX-Board/Libraries/TinyGPSPlus" -I../SubGHz_Phy/App -I../SubGHz_Phy/Target -I../Utilities/trace/adv_trace -I../Utilities/misc -I../Utilities/sequencer -I../Utilities/timer -I../Utilities/lpm/tiny_lpm -I../Middlewares/Third_Party/SubGHz_Phy -I../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver -I"D:/GitHub/guillermo-loradesign-fw/Firmware-Development/STM32CubeIDE/TX-Board/Application" -I"D:/GitHub/guillermo-loradesign-fw/Firmware-Development/STM32CubeIDE/TX-Board/Components/GPS" -I"D:/GitHub/guillermo-loradesign-fw/Firmware-Development/STM32CubeIDE/TX-Board/Components/MCP7940" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Components-2f-MCP7940

clean-Components-2f-MCP7940:
	-$(RM) ./Components/MCP7940/MCP7940.cyclo ./Components/MCP7940/MCP7940.d ./Components/MCP7940/MCP7940.o ./Components/MCP7940/MCP7940.su

.PHONY: clean-Components-2f-MCP7940

