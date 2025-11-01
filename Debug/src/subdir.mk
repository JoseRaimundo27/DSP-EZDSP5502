################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ibit_rev.c \
../src/intrinsic_fft.c \
../src/intrinsic_fftTest.c 

OBJS += \
./src/ibit_rev.obj \
./src/intrinsic_fft.obj \
./src/intrinsic_fftTest.obj 

C_DEPS += \
./src/ibit_rev.pp \
./src/intrinsic_fft.pp \
./src/intrinsic_fftTest.pp 

C_DEPS__QUOTED += \
"src\ibit_rev.pp" \
"src\intrinsic_fft.pp" \
"src\intrinsic_fftTest.pp" 

OBJS__QUOTED += \
"src\ibit_rev.obj" \
"src\intrinsic_fft.obj" \
"src\intrinsic_fftTest.obj" 

C_SRCS__QUOTED += \
"../src/ibit_rev.c" \
"../src/intrinsic_fft.c" \
"../src/intrinsic_fftTest.c" 


# Each subdirectory must supply rules for building sources it contributes
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

src/intrinsic_fftTest.obj: ../src/intrinsic_fftTest.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/intrinsic_fftTest.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


