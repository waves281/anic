main: start anic.exe
	
all: start cleanout anic.exe

start:
	@echo anic ANI Compiler Makefile
	
anic.exe: Makefile temp/version.exe var/lexerStruct.h bld/hexTruncate.awk \
		src/mainDefs.h src/constantDefs.h src/globalVars.h \
		src/system.h src/customOperators.h src/lexer.h \
		src/core.cpp src/system.cpp src/customOperators.cpp src/lexer.cpp
	@echo Building main executable...
	@g++ src/core.cpp src/system.cpp src/customOperators.cpp src/lexer.cpp \
	-D BUILD_NUMBER_MAIN="\"`./temp/version.exe`\"" \
	-D BUILD_NUMBER_SUB="\"` date | crypt password | awk -f bld/hexTruncate.awk `\"" \
	-o anic.exe \
	-O3 \
	-Wall
	@echo Done.
	
temp/version.exe: bld/version.c src/mainDefs.h src/constantDefs.h
	@echo Building version controller...
	@mkdir -p var
	@mkdir -p temp
	@gcc bld/version.c -o temp/version.exe
	
var/lexerStruct.h: temp/lexerStructGen.exe src/lexerTable.txt
	@echo Generating lexer structure...
	@mkdir -p var
	@./temp/lexerStructGen.exe

temp/lexerStructGen.exe: bld/lexerStructGen.c
	@echo Building lexer structure generator...
	@mkdir -p temp
	@gcc bld/lexerStructGen.c -o temp/lexerStructGen.exe

clean: cleanout
	@echo Cleaning temporary files...
	@rm -R -f var
	@rm -R -f temp
	
cleanout:
	@echo Cleaning output...
	@rm -f anic.exe
	@rm -f temp/version.exe
	@rm -f temp/lexerStructGen.exe
