# Compiler configuration
CC = /var/home/waffles/code/cosmocc/bin/cosmocc
CFLAGS = -Wall -Wextra -std=c99 -O0 -fno-debug-types-section -I./SDL3/include
LDFLAGS = -ldl

# Project settings
BIN = cosmo-sdl
BINDIR = bin
OBJDIR = obj

# Source files
SRCS = main.c
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

# Link executable
$(BINDIR)/$(BIN): $(OBJS) | $(BINDIR)
	@echo "LD $@"
	@$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@rm -rf $(OBJDIR)

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean