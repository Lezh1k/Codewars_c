PRG = fib

SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = bin

SRC := $(wildcard $(SRC_DIR)/*.s) $(wildcard $(SRC_DIR)/*.asm)

OBJECTS := $(patsubst %.s,   $(OBJ_DIR)/%.o, $(filter %.s,   $(SRC)))
OBJECTS += $(patsubst %.asm, $(OBJ_DIR)/%.o, $(filter %.asm, $(SRC)))

LDFLAGS := -lc --dynamic-linker /lib64/ld-linux-x86-64.so.2

all: directories $(PRG)

$(PRG): $(BIN_DIR)/$(PRG)

$(OBJ_DIR)/%.o: %.s
	@mkdir -p $(@D)
	@nasm -f elf64 -o $@ $<

$(OBJ_DIR)/%.o: %.asm
	@mkdir -p $(@D)
	@nasm -f elf64 -o $@ $<

$(BIN_DIR)/$(PRG): $(OBJECTS)
	@mkdir -p $(@D)
	@ld $^ -o $@ $(LDFLAGS)

.PHONY: directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)/*
	@rm -rf $(BIN_DIR)/*

.PHONY: mrproper
mrproper:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BIN_DIR)
