################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/FFT_iFFT_Test.c \
../src/ibit_rev.c \
../src/intrinsic_fft.c \
../src/w_table.c 

OBJS += \
./src/FFT_iFFT_Test.obj \
./src/ibit_rev.obj \
./src/intrinsic_fft.obj \
./src/w_table.obj 

C_DEPS += \
./src/FFT_iFFT_Test.pp \
./src/ibit_rev.pp \
./src/intrinsic_fft.pp \
./src/w_table.pp 

C_DEPS__QUOTED += \
"src\FFT_iFFT_Test.pp" \
"src\ibit_rev.pp" \
"src\intrinsic_fft.pp" \
"src\w_table.pp" 

OBJS__QUOTED += \
"src\FFT_iFFT_Test.obj" \
"src\ibit_rev.obj" \
"src\intrinsic_fft.obj" \
"src\w_table.obj" 

C_SRCS__QUOTED += \
"../src/FFT_iFFT_Test.c" \
"../src/ibit_rev.c" \
"../src/intrinsic_fft.c" \
"../src/w_table.c" 


# Each subdirectory must supply rules for building sources it contributes
src/FFT_iFFT_Test.obj: ../src/FFT_iFFT_Test.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/FFT_iFFT_Test.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/ibit_rev.obj: ../src/ibit_rev.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/ibit_rev.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/intrinsic_fft.obj: ../src/intrinsic_fft.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/intrinsic_fft.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/w_table.obj: ../src/w_table.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/w_table.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


