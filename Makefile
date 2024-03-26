# Directories
SRC_DIR := src
INCLUDE_DIR := include
TESTS_DIR := tests
BUILD_DIR := build
LIB_NAME := mcucli


# Compiler and flags
CC := gcc
AR := ar
CFLAGS := -Wall -Wextra -I$(INCLUDE_DIR)
LD_FLAGS := -L$(BUILD_DIR) -l$(LIB_NAME)

# Source files
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

# Test files
TEST_FILES := $(wildcard $(TESTS_DIR)/*.c)
TEST_EXECS := $(patsubst $(TESTS_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_FILES))

# Targets
all: lib test

lib: $(OBJ_FILES)
	$(AR) rcs $(BUILD_DIR)/lib$(LIB_NAME).a $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_EXECS)

$(BUILD_DIR)/%: $(TESTS_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@ $(LD_FLAGS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all lib test clean