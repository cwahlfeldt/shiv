# Compiler configuration
CC = /var/home/waffles/code/cosmocc/bin/cosmocc
CFLAGS = -Wall -Wextra -std=c99 -O0 -fno-debug-types-section -I./modules/SDL/include
LDFLAGS = -ldl

# Project settings
BIN = shiv
PLUGIN = plugin
BINDIR = build
OBJDIR = obj

# Source files
SRCS = src/main.c
PLUGIN_SRCS = src/plugin.c
OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(SRCS))
PLUGIN_OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(PLUGIN_SRCS))

# Default target
all: $(BINDIR)/$(BIN) $(BINDIR)/$(PLUGIN)

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

# Link plugin executable
$(BINDIR)/$(PLUGIN): $(PLUGIN_OBJS) | $(BINDIR)
	@echo "LD $@"
	@$(CC) $(CFLAGS) $(PLUGIN_OBJS) -o $@ $(LDFLAGS)

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean