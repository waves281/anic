TARGET = anic.exe
INSTALL_PATH = /usr/bin
TEST_FILES = tst/test.ani



### /* BUILD TYPES */

main: start $(TARGET)

all: start cleanout test install

install: $(TARGET)
	@echo Installing...
	@cp -f $(TARGET) $(INSTALL_PATH)/$(TARGET)
	@echo Done installing.

uninstall: 
	@echo Uninstalling...
	@rm -f $(INSTALL_PATH)/$(TARGET)

clean: cleanout
	@echo Cleaning temporary files...
	@rm -R -f var
	@rm -R -f tmp

cleanout:
	@echo Cleaning output...
	@rm -f $(TARGET)
	@rm -f tmp/version.exe
	@rm -f tmp/lexerStructGen.exe
	@rm -f tmp/parserStructGen.exe
	@make --directory=bld/hyacc --makefile=makefile clean -s

purge: uninstall clean



### /* DEPENDENCIES */

start:
	@echo anic ANI Compiler Makefile
	@echo

$(TARGET): tmp/version.exe var/lexerStruct.h var/parserStruct.h bld/hexTruncate.awk \
		src/mainDefs.h src/constantDefs.h src/globalVars.h \
		src/system.h src/customOperators.h src/lexer.h src/parser.h \
		src/core.cpp src/system.cpp src/customOperators.cpp var/lexerStruct.cpp src/lexer.cpp src/parser.cpp
	@echo Building main executable...
	@g++ src/core.cpp src/system.cpp src/customOperators.cpp var/lexerStruct.cpp src/lexer.cpp src/parser.cpp \
	-D BUILD_NUMBER_MAIN="\"`./tmp/version.exe`\"" \
	-D BUILD_NUMBER_SUB="\"` date | crypt password | awk -f bld/hexTruncate.awk `\"" \
	-o $(TARGET) \
	-O3 \
	-fomit-frame-pointer \
	-ffast-math \
	-pipe \
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
	
tmp/parserStructGen.exe: bld/parserStructGen.cpp
	@echo Building parser structure generator...
	@mkdir -p tmp
	@g++ bld/parserStructGen.cpp -o tmp/parserStructGen.exe

var/parserTable.txt: tmp/hyacc.exe src/parserGrammar.y
	@echo Generating parser table...
	@./tmp/hyacc.exe -c -v -D1 -D2 -O1 -Q src/parserGrammar.y
	@mv y.OUTPUT var/parserTable.txt

tmp/hyacc.exe: bld/hyacc/makefile
	@echo Building hyacc parse table generator...
	@make --directory=bld/hyacc --makefile=makefile -s

test: $(TARGET)
	@echo
	@echo ...Running default test cases...
	@echo --------------------------------
	./$(TARGET) -v $(TEST_FILES)
	@echo --------------------------------
	@echo Done running default test cases.
