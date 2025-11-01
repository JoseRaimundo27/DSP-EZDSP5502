################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/asm_dft128Test.c 

ASM_SRCS += \
../src/dft128.asm \
../src/mag_128.asm \
../src/sine_cos.asm 

ASM_DEPS += \
./src/dft128.pp \
./src/mag_128.pp \
./src/sine_cos.pp 

OBJS += \
./src/asm_dft128Test.obj \
./src/dft128.obj \
./src/mag_128.obj \
./src/sine_cos.obj 

C_DEPS += \
./src/asm_dft128Test.pp 

ASM_DEPS__QUOTED += \
"src\dft128.pp" \
"src\mag_128.pp" \
"src\sine_cos.pp" 

C_DEPS__QUOTED += \
"src\asm_dft128Test.pp" 

OBJS__QUOTED += \
"src\asm_dft128Test.obj" \
"src\dft128.obj" \
"src\mag_128.obj" \
"src\sine_cos.obj" 

C_SRCS__QUOTED += \
"../src/asm_dft128Test.c" 

ASM_SRCS__QUOTED += \
"../src/dft128.asm" \
"../src/mag_128.asm" \
"../src/sine_cos.asm" 


# Each subdirectory must supply rules for building sources it contributes
src/asm_dft128Test.obj: ../src/asm_dft128Test.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/asm_dft128Test.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/dft128.obj: ../src/dft128.asm $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/dft128.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/mag_128.obj: ../src/mag_128.asm $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/mag_128.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/sine_cos.obj: ../src/sine_cos.asm $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/sine_cos.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


