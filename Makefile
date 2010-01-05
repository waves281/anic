TARGET = anic.exe
INSTALL_PATH = /usr/bin

TEST_FILES = tst/test.ani



### BUILD TYPES

main: start $(TARGET)

all: start cleanout test install

install: start $(TARGET)
	@echo Installing...
	@cp -f $(TARGET) $(INSTALL_PATH)/$(TARGET)
	@echo Done installing.

uninstall: start
	@echo Uninstalling...
	@rm -f $(INSTALL_PATH)/$(TARGET)

clean: start cleanout
	@echo Cleaning temporary files...
	@rm -R -f var
	@rm -R -f tmp

cleanout: start
	@echo Cleaning output...
	@rm -f $(TARGET)
	@rm -f tmp/version.exe
	@rm -f tmp/lexerStructGen.exe
	@rm -f tmp/parserStructGen.exe
	@make --directory=bld/hyacc --makefile=makefile clean -s

purge: start uninstall clean



### WRAPPER RULES

start:
	@echo anic ANI Compiler Makefile
	@echo

test: $(TARGET)
	@echo
	@echo ...Running default test cases...
	@echo --------------------------------
	./$(TARGET) -v $(TEST_FILES)
	@echo --------------------------------
	@echo Done running default test cases.



### BUILD AUXILIARIES

tmp/version.exe: bld/version.c
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

tmp/hyacc.exe: bld/hyacc/makefile
	@echo Building parser table generator...
	@make --directory=bld/hyacc --makefile=makefile -s

var/parserTable.txt: tmp/hyacc.exe src/parserGrammar.y
	@echo Generating parser table...
	@./tmp/hyacc.exe -c -v -D1 -D2 -O1 -Q src/parserGrammar.y
	@mv y.output var/parserTable.txt



### CORE APPLICATION

$(TARGET): tmp/version.exe var/lexerStruct.h var/parserStruct.h bld/hexTruncate.awk \
		src/mainDefs.h src/constantDefs.h src/system.h src/customOperators.h \
		src/lexer.h src/parser.h src/semmer.h \
		src/core.cpp src/system.cpp src/customOperators.cpp var/lexerStruct.cpp src/lexer.cpp src/parser.cpp src/semmer.cpp
	@echo Building main executable...
	@g++ src/core.cpp src/system.cpp src/customOperators.cpp var/lexerStruct.cpp src/lexer.cpp src/parser.cpp src/semmer.cpp \
	-D BUILD_NUMBER_MAIN="\"`./tmp/version.exe`\"" \
	-D BUILD_NUMBER_SUB="\"` date | crypt password | awk -f bld/hexTruncate.awk `\"" \
	-o $(TARGET) \
	-O3 \
	-s \
	-fomit-frame-pointer \
	-ffast-math \
	-pipe \
	-Wall
	@echo Done building main executable.
