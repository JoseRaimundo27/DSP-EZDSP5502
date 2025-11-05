################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
lib/%.obj: ../lib/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C5500 Compiler'
	"/home/caco/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1/bin/cl55" -v5502 --memory_model=small -g --include_path="/home/caco/workspace_v12/DMA" --include_path="/home/caco/workspace_v12/DMA/include" --include_path="/home/caco/Área de trabalho/EngComp/LABIII/ezdsp5502_BSL_RevC/ezdsp5502_v1/C55xxCSL/include" --include_path="/home/caco/Área de trabalho/EngComp/LABIII/ezdsp5502_BSL_RevC/ezdsp5502_v1/include" --include_path="/home/caco/ti/ccs1281/ccs/tools/compiler/c5500_4.4.1/include" --define=c5502 --define=CHIP_5502 --display_error_number --diag_warning=225 --ptrdiff_size=16 --preproc_with_compile --preproc_dependency="lib/$(basename $(<F)).d_raw" --obj_directory="lib" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


