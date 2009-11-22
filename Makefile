main: start anic.exe
	
all: start cleanout anic.exe

start:
	@echo anic ANI Compiler Makefile
	
anic.exe: Makefile temp/version.exe src/core.cpp src/system.h src/mainDefs.h src/constantDefs.h src/globalVars.h
	@echo Building main executable...
	@g++ ./src/core.cpp \
	-D BUILD_NUMBER_MAIN="\"`./temp/version.exe`\"" \
	-D BUILD_NUMBER_SUB="\"`date +%s`\"" \
	-o anic.exe \
	-O3 \
	-Wall
	@echo Done.
	
temp/version.exe: src/version.c
	@echo Building version controller...
	@mkdir -p var
	@mkdir -p temp
	@gcc src/version.c -o temp/version.exe

clean: cleanout
	@echo Cleaning temporary files...
	@rm -R -f var
	@rm -R -f temp
	
cleanout:
	@echo Cleaning output...
	@rm -f anic.exe
	@rm -f temp/version.exe
