################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := E:/MyGame/Tools/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./ti_msp_dl_config.o" \
"./main-blinky.o" \
"./main.o" \
"./Driver/I2C.o" \
"./Driver/motor.o" \
"./Driver/pid.o" \
"./Driver/sys.o" \
"./Driver/uart.o" \
"./freertos/ticlang/startup_mspm0g350x_ticlang.o" \
"../mspm0g3507.cmd" \
$(GEN_CMDS__FLAG) \
-Wl,-lfreertos_builds_LP_MSPM0G3507_release_ticlang.lib \
-Wl,-l"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/source/ti/driverlib/lib/ticlang/m0p/mspm0g1x0x_g3x0x/driverlib.a" \
-Wl,-llibc.a \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include Driver/subdir_vars.mk
-include freertos/ticlang/subdir_vars.mk
-include subdir_rules.mk
-include Driver/subdir_rules.mk
-include freertos/ticlang/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
car.out 

EXE_OUTPUTS__QUOTED += \
"car.out" 


# All Target
all: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "car.out"

dependents:
	-cd D:/AiliuCheng/work/DIAN_RACE/car/freertos_builds_LP_MSPM0G3507_release_ticlang/Debug && $(MAKE) clean all

# Tool invocations
car.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS) D:/AiliuCheng/work/DIAN_RACE/car/freertos_builds_LP_MSPM0G3507_release_ticlang/Debug/freertos_builds_LP_MSPM0G3507_release_ticlang.lib
	@echo 'Building target: "$@"'
	@echo 'Invoking: Arm Linker'
	"E:/MyGame/Tools/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS/bin/tiarmclang.exe" -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -D__MSPM0G3507__ -gdwarf-3 -Wall -Wl,-m"car.map" -Wl,-i"D:/AiliuCheng/work/DIAN_RACE/car/freertos_builds_LP_MSPM0G3507_release_ticlang/Debug" -Wl,-i"D:/AiliuCheng/work/DIAN_RACE/TiSDK/mspm0_sdk_2_10_00_04/source" -Wl,-i"D:/AiliuCheng/work/DIAN_RACE/car/car" -Wl,-i"D:/AiliuCheng/work/DIAN_RACE/car/car/Debug/syscfg" -Wl,-i"E:/MyGame/Tools/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS/lib" -Wl,--diag_wrap=off -Wl,--display_error_number -Wl,--warn_sections -Wl,--xml_link_info="car_linkInfo.xml" -Wl,--rom_model -o "car.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(GEN_MISC_FILES__QUOTED)$(GEN_FILES__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "ti_msp_dl_config.o" "main-blinky.o" "main.o" "Driver\I2C.o" "Driver\motor.o" "Driver\pid.o" "Driver\sys.o" "Driver\uart.o" "freertos\ticlang\startup_mspm0g350x_ticlang.o" 
	-$(RM) "ti_msp_dl_config.d" "main-blinky.d" "main.d" "Driver\I2C.d" "Driver\motor.d" "Driver\pid.d" "Driver\sys.d" "Driver\uart.d" "freertos\ticlang\startup_mspm0g350x_ticlang.d" 
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
D:/AiliuCheng/work/DIAN_RACE/car/freertos_builds_LP_MSPM0G3507_release_ticlang/Debug/freertos_builds_LP_MSPM0G3507_release_ticlang.lib:

-include ../makefile.targets

