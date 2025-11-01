################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/float_dft128.c \
../src/float_dft128Test.c \
../src/float_mag128.c 

OBJS += \
./src/float_dft128.obj \
./src/float_dft128Test.obj \
./src/float_mag128.obj 

C_DEPS += \
./src/float_dft128.pp \
./src/float_dft128Test.pp \
./src/float_mag128.pp 

C_DEPS__QUOTED += \
"src\float_dft128.pp" \
"src\float_dft128Test.pp" \
"src\float_mag128.pp" 

OBJS__QUOTED += \
"src\float_dft128.obj" \
"src\float_dft128Test.obj" \
"src\float_mag128.obj" 

C_SRCS__QUOTED += \
"../src/float_dft128.c" \
"../src/float_dft128Test.c" \
"../src/float_mag128.c" 


# Each subdirectory must supply rules for building sources it contributes
src/float_dft128.obj: ../src/float_dft128.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/float_dft128.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/float_dft128Test.obj: ../src/float_dft128Test.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/float_dft128Test.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/float_mag128.obj: ../src/float_mag128.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/float_mag128.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


