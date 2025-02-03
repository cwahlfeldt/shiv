# Compiler settings
CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -g
CFLAGS += -Ilib/SDL/include
CFLAGS += -Iinclude
CFLAGS += -Wno-unused-parameter
OUT := build

# Platform detection and settings
ifeq ($(OS),Windows_NT)
    detected_OS := Windows
    TARGET := shiv.exe
    # Windows-specific settings
    LDFLAGS := -Llib/SDL/build -lSDL3
    # Windows command settings
    RM := del /Q /F
    RMDIR := rmdir /Q /S
    MKDIR := mkdir
    PATH_SEP := \\
else
    detected_OS := $(shell uname -s)
    
    ifeq ($(detected_OS),Darwin)
        # Mac-specific settings
        TARGET := shiv
        LDFLAGS := -Llib/SDL/build -lSDL3
        CFLAGS += -D_REENTRANT
        # Mac often needs these frameworks
        LDFLAGS += -framework CoreVideo -framework CoreAudio -framework AudioToolbox
        # Use @rpath for dynamic library loading on Mac
        LDFLAGS += -Wl,-rpath,@executable_path/lib/SDL/build
    else
        # Linux settings
        TARGET := shiv
        LDFLAGS := -Llib/SDL/build -lSDL3
        CFLAGS += -D_REENTRANT
        LDFLAGS += -Wl,-rpath,$(PWD)/lib/SDL/build
    endif
    
    # Unix-like command settings (both Mac and Linux)
    RM := rm -f
    RMDIR := rm -rf
    MKDIR := mkdir -p
    PATH_SEP := /
endif

# Convert paths to platform-specific format
OUT_DIR := $(subst /,$(PATH_SEP),$(OUT))

# Source files
SRCS := src/main.c src/shiv.c
OBJS := $(patsubst src/%.c,$(OUT)/%.o,$(SRCS))

# Normalize paths for Windows
ifeq ($(detected_OS),Windows)
    OBJS := $(subst /,\,$(OBJS))
    TARGET := $(subst /,\,$(TARGET))
endif

# Targets
.PHONY: all clean run info

all: $(OUT)/$(TARGET)

$(OUT):
	@$(MKDIR) $(OUT_DIR)

$(OUT)/%.o: src/%.c | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

run: $(OUT)/$(TARGET)
ifeq ($(detected_OS),Windows)
	$(OUT_DIR)\\$(TARGET)
else
	./$(OUT)/$(TARGET)
endif

clean:
ifeq ($(detected_OS),Windows)
	@if exist "$(OUT_DIR)" $(RMDIR) "$(OUT_DIR)"
else
	$(RMDIR) $(OUT)
endif
