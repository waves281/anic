main: start anic.exe

all: start cleanout test

start:
	@echo anic ANI Compiler Makefile
	@echo

anic.exe: Makefile tmp/version.exe var/lexerStruct.h var/grammarStruct.h bld/hexTruncate.awk \
		src/mainDefs.h src/constantDefs.h src/globalVars.h \
		src/system.h src/customOperators.h src/lexer.h \
		src/core.cpp src/system.cpp src/customOperators.cpp src/lexer.cpp
	@echo Building main executable...
	@g++ src/core.cpp src/system.cpp src/customOperators.cpp src/lexer.cpp \
	-D BUILD_NUMBER_MAIN="\"`./tmp/version.exe`\"" \
	-D BUILD_NUMBER_SUB="\"` date | crypt password | awk -f bld/hexTruncate.awk `\"" \
	-o anic.exe \
	-O3 \
	-Wall
	@echo Done building main executable.

tmp/version.exe: bld/version.c src/mainDefs.h src/constantDefs.h
	@echo Building version controller...
	@mkdir -p var
	@mkdir -p tmp
	@gcc bld/version.c -o tmp/version.exe

var/lexerStruct.h: tmp/lexerStructGen.exe src/lexerTable.txt
	@echo Generating lexer structure...
	@mkdir -p var
	@./tmp/lexerStructGen.exe

tmp/lexerStructGen.exe: bld/lexerStructGen.c
	@echo Building lexer structure generator...
	@mkdir -p tmp
	@gcc bld/lexerStructGen.c -o tmp/lexerStructGen.exe

var/grammarStruct.h: bld/hyacc/hyacc.exe src/grammar.y
	@echo Generating parser structure...
	@./bld/hyacc/hyacc.exe -c -v -D1 -D2 -O1 -Q src/grammar.y
	@mv y.OUTPUT var/grammarStruct.h

bld/hyacc/hyacc.exe: bld/hyacc/makefile
	@echo Building hyacc parser generator...
	@make --directory=bld/hyacc --makefile=makefile -s

test: anic.exe
	@echo
	@echo ...Running default test cases...
	@echo --------------------------------
	./anic.exe -v ./tst/test.ani
	@echo --------------------------------
	@echo Done running test cases.

clean: cleanout
	@echo Cleaning temporary files...
	@rm -R -f var
	@rm -R -f tmp
	
cleanout:
	@echo Cleaning output...
	@rm -f anic.exe
	@rm -f tmp/version.exe
	@rm -f tmp/lexerStructGen.exe
	@make --directory=bld/hyacc --makefile=makefile clean -s
