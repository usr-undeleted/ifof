CC             ="clang"
BIN_OUTPUT     ="$(pwd)bin/"
EMU_BIN_NAME   ="ifof_emu"
ASM_BIN_NAME   ="ifof_asm"
SRC            ="$(pwd)src"
CC_FLAGS       ="-Wextra" "-Wall" "-std=gnu23"
INVOC_ARGS     =""

.PHONY: clean

emulator:
	@mkdir "-p" $(BIN_OUTPUT)
	@$(CC) "-o" $(BIN_OUTPUT)$(EMU_BIN_NAME) "-I" $(SRC)/emu $(SRC)/emu/*.c $(CC_FLAGS) "-I$(SRC)" $(SRC)/flag.c

assembler:
	@mkdir "-p" $(BIN_OUTPUT)
	@$(CC) "-o" $(BIN_OUTPUT)$(ASM_BIN_NAME) "-I" $(SRC)/asm $(SRC)/asm/*.c $(CC_FLAGS) "-I$(SRC)" $(SRC)/flag.c

all: emulator assembler

clean:
	@rm -r $(BIN_OUTPUT)

sequence: compile run
