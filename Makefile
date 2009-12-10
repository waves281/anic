main: start anic.exe

all: start cleanout test

start:
	@echo anic ANI Compiler Makefile
	@echo

anic.exe: Makefile tmp/version.exe var/lexerStruct.h var/parserStruct.h bld/hexTruncate.awk \
		src/mainDefs.h src/constantDefs.h src/globalVars.h \
		src/system.h src/customOperators.h src/lexer.h src/parser.h \
		src/core.cpp src/system.cpp src/customOperators.cpp var/lexerStruct.cpp src/lexer.cpp src/parser.cpp
	@echo Building main executable...
	@g++ src/core.cpp src/system.cpp src/customOperators.cpp var/lexerStruct.cpp src/lexer.cpp \
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

var/lexerStruct.h var/lexerStruct.cpp: tmp/lexerStructGen.exe src/lexerTable.txt
	@echo Generating lexer structures...
	@mkdir -p var
	@./tmp/lexerStructGen.exe

tmp/lexerStructGen.exe: bld/lexerStructGen.cpp
	@echo Building lexer structure generator...
	@mkdir -p tmp
	@g++ bld/lexerStructGen.cpp -o tmp/lexerStructGen.exe

var/parserStruct.h: tmp/parserStructGen.exe var/parserTable.txt
	@echo Generating parser structure...
	@mkdir -p var
	@./tmp/parserStructGen.exe
	
tmp/parserStructGen.exe: 
	@echo Building parser structure generator...
	@mkdir -p tmp
	@gcc bld/parserStructGen.c -o tmp/parserStructGen.exe

var/parserTable.txt: bld/hyacc/hyacc.exe src/parserGrammar.y
	@echo Generating parser table...
	@./tmp/hyacc.exe -c -v -D1 -D2 -O1 -Q src/parserGrammar.y
	@mv y.OUTPUT var/parserTable.txt

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
	@rm -f tmp/parserStructGen.exe
	@make --directory=bld/hyacc --makefile=makefile clean -s
