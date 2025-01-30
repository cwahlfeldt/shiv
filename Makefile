# Compiler configuration
CC = /var/home/waffles/code/cosmocc/bin/cosmocc
CFLAGS = -Wall -Wextra -std=c99 -O0 -Wno-implicit-function-declaration -fno-debug-types-section -I./lib/SDL/include
LDFLAGS = -ldl

# Project settings
BIN = shiv
BINDIR = build
OBJDIR = obj

# Source files
SRCS = src/main.c src/shiv_sdl.c
OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(SRCS))

# Default target
all: $(BINDIR)/$(BIN)

# Object file compilation
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@mkdir -p '$(@D)'
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Create directories
$(OBJDIR) $(BINDIR):
	@mkdir -p $@

# Link main executable
$(BINDIR)/$(BIN): $(OBJS) | $(BINDIR)
	@echo "LD $@"
	@$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean