################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-1423402465: ../car.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"D:/AiliuCheng/work/DIAN_RACE/TiSDK/sysconfig_cli.bat" --script "D:/AiliuCheng/work/DIAN_RACE/car/car/car.syscfg" -o "." -s "D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/.metadata/product.json" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

ti_msp_dl_config.c: build-1423402465 ../car.syscfg
ti_msp_dl_config.h: build-1423402465
Event.dot: build-1423402465

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"E:/MyGame/Tools/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS/bin/tiarmclang.exe" -c -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"D:/AiliuCheng/work/DIAN_RACE/car/car" -I"D:/AiliuCheng/work/DIAN_RACE/car/car/Debug" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/kernel/freertos/Source/include" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/source" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/kernel/freertos/Source/portable/TI_ARM_CLANG/ARM_CM0" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/source/ti/posix/ticlang" -I"D:/AiliuCheng/work/DIAN_RACE/car/freertos_builds_LP_MSPM0G3507_release_ticlang" -I"D:/AiliuCheng/work/DIAN_RACE/car/car/Driver" -I"D:/AiliuCheng/work/DIAN_RACE/car/car/oled" -D__MSPM0G3507__ -gdwarf-3 -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"E:/MyGame/Tools/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS/bin/tiarmclang.exe" -c -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"D:/AiliuCheng/work/DIAN_RACE/car/car" -I"D:/AiliuCheng/work/DIAN_RACE/car/car/Debug" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/kernel/freertos/Source/include" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/source" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/kernel/freertos/Source/portable/TI_ARM_CLANG/ARM_CM0" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/source/ti/posix/ticlang" -I"D:/AiliuCheng/work/DIAN_RACE/car/freertos_builds_LP_MSPM0G3507_release_ticlang" -I"D:/AiliuCheng/work/DIAN_RACE/car/car/Driver" -I"D:/AiliuCheng/work/DIAN_RACE/car/car/oled" -D__MSPM0G3507__ -gdwarf-3 -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


