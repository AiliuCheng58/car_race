################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
oled/%.o: ../oled/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"E:/MyGame/Tools/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS/bin/tiarmclang.exe" -c -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"D:/AiliuCheng/work/DIAN_RACE/car/car" -I"D:/AiliuCheng/work/DIAN_RACE/car/car/Debug" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/kernel/freertos/Source/include" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/source" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/kernel/freertos/Source/portable/TI_ARM_CLANG/ARM_CM0" -I"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/source/ti/posix/ticlang" -I"D:/AiliuCheng/work/DIAN_RACE/car/freertos_builds_LP_MSPM0G3507_release_ticlang" -I"D:/AiliuCheng/work/DIAN_RACE/car/car/Driver" -I"D:/AiliuCheng/work/DIAN_RACE/car/car/oled" -D__MSPM0G3507__ -gdwarf-3 -Wall -MMD -MP -MF"oled/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


