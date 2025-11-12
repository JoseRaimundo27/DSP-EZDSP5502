################################################################################
# Automatically-generated file. Do not edit!
################################################################################

CG_TOOL_ROOT := /home/caco/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./aic3204.obj" \
"./dma.obj" \
"./main.obj" \
"./oled.obj" \
"./vectors_dma4.obj" \
"./fft/ibit_rev.obj" \
"./fft/intrinsic_fft.obj" \
"./include/ezdsp5502.obj" \
"./include/ezdsp5502_gpio.obj" \
"./include/ezdsp5502_i2c.obj" \
"./include/ezdsp5502_i2cgpio.obj" \
"./include/ezdsp5502_mcbsp.obj" \
"./include/lcd.obj" \
"../lnkx.cmd" \
$(GEN_CMDS__FLAG) \
-llibc.a \
-l"/home/caco/Área de trabalho/EngComp/LABIII/ezdsp5502_BSL_RevC/ezdsp5502_v1/C55xxCSL/lib/csl5502x.lib" \

-include ../makefile.init

RM := rm -rf
RMDIR := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include fft/subdir_vars.mk
-include include/subdir_vars.mk
-include subdir_rules.mk
-include fft/subdir_rules.mk
-include include/subdir_rules.mk
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
aic3204_dma_flanger.out 

EXE_OUTPUTS__QUOTED += \
"aic3204_dma_flanger.out" 


# All Target
all: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "aic3204_dma_flanger.out"

# Tool invocations
aic3204_dma_flanger.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: C5500 Linker'
	"/home/caco/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1/bin/cl55" -v5502 --memory_model=large -g --define=c5502 --display_error_number --diag_warning=225 --sat_reassoc=on --ptrdiff_size=32 -z -m"aic3204_dma_flanger.map" --stack_size=0x200 --heap_size=0x400 -i"/home/caco/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1/lib" -i"/home/caco/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1/include" --reread_libs --display_error_number --warn_sections --xml_link_info="aic3204_dma_flanger_linkInfo.xml" --rom_model --sys_stacksize=0x200 -o "aic3204_dma_flanger.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)
	-$(RM) "aic3204.obj" "dma.obj" "main.obj" "oled.obj" "vectors_dma4.obj" "fft/ibit_rev.obj" "fft/intrinsic_fft.obj" "include/ezdsp5502.obj" "include/ezdsp5502_gpio.obj" "include/ezdsp5502_i2c.obj" "include/ezdsp5502_i2cgpio.obj" "include/ezdsp5502_mcbsp.obj" "include/lcd.obj" 
	-$(RM) "vectors_dma4.d" 
	-$(RM) "aic3204.d" "dma.d" "main.d" "oled.d" "fft/ibit_rev.d" "fft/intrinsic_fft.d" "include/ezdsp5502.d" "include/ezdsp5502_gpio.d" "include/ezdsp5502_i2c.d" "include/ezdsp5502_i2cgpio.d" "include/ezdsp5502_mcbsp.d" "include/lcd.d" 
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

