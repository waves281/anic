main: start anic.exe
	
all: start cleanout anic.exe

start:
	@echo anic ANI Compiler Makefile
	
anic.exe: Makefile temp/version.exe var/lexerStruct.h src/core.cpp src/system.h src/mainDefs.h src/constantDefs.h src/globalVars.h src/lexer.h src/lexer.cpp
	@echo Building main executable...
	@g++ ./src/core.cpp ./src/system.cpp ./src/lexer.cpp \
	-D BUILD_NUMBER_MAIN="\"`./temp/version.exe`\"" \
	-D BUILD_NUMBER_SUB="\"`date +%s`\"" \
	-o anic.exe \
	-O3 \
	-Wall
	@echo Done.
	
temp/version.exe: src/version.c src/constantDefs.h
	@echo Building version controller...
	@mkdir -p var
	@mkdir -p temp
	@gcc src/version.c -o temp/version.exe
	
var/lexerStruct.h: temp/lexerStructGen.exe src/lexerTable.txt
	@echo Generating lexer structure...
	@mkdir -p var
	@./temp/lexerStructGen.exe

temp/lexerStructGen.exe: src/lexerStructGen.c
	@echo Building lexer structure generator...
	@mkdir -p temp
	@gcc src/lexerStructGen.c -o temp/lexerStructGen.exe

clean: cleanout
	@echo Cleaning temporary files...
	@rm -R -f var
	@rm -R -f temp
	
cleanout:
	@echo Cleaning output...
	@rm -f anic.exe
	@rm -f temp/version.exe
	@rm -f temp/lexerStructGen.exe
