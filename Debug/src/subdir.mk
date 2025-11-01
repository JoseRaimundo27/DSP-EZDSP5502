################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/fbit_rev.c \
../src/fft_float.c \
../src/float_fftTest.c 

OBJS += \
./src/fbit_rev.obj \
./src/fft_float.obj \
./src/float_fftTest.obj 

C_DEPS += \
./src/fbit_rev.pp \
./src/fft_float.pp \
./src/float_fftTest.pp 

C_DEPS__QUOTED += \
"src\fbit_rev.pp" \
"src\fft_float.pp" \
"src\float_fftTest.pp" 

OBJS__QUOTED += \
"src\fbit_rev.obj" \
"src\fft_float.obj" \
"src\float_fftTest.obj" 

C_SRCS__QUOTED += \
"../src/fbit_rev.c" \
"../src/fft_float.c" \
"../src/float_fftTest.c" 


# Each subdirectory must supply rules for building sources it contributes
src/fbit_rev.obj: ../src/fbit_rev.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/fbit_rev.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/fft_float.obj: ../src/fft_float.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/fft_float.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/float_fftTest.obj: ../src/float_fftTest.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/float_fftTest.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


