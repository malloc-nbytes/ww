# Compiler and flags
CC      := cc

# Compilation flags (release)
CFLAGS  := -Iinclude \
           -O2 \
           -Wall \
           -Wextra \
           -pedantic \
           -std=gnu17 \
           -Wshadow \
           -Wconversion \
           -Wsign-conversion \
           -Wunused \
           -Wuninitialized \
           -Wstrict-prototypes \
           -Wmissing-prototypes \
           -Wmissing-declarations \
           -Wredundant-decls \
           -Wfloat-equal \
           -Wdouble-promotion \
           -Wformat=2 \
           -Wformat-truncation \
           -Wformat-overflow \
           -Wundef \
           -Wcast-align \
           -Wcast-qual \
           -Wpointer-arith \
           -fstack-protector-strong \
           -D_FORTIFY_SOURCE=2

# Debug flags
DEBUG_FLAGS := -O0 -g3

# Linker flags
LDFLAGS :=

# Directories
SRCDIR   := .
OBJDIR   := obj
BINDIR   := bin
INCLUDE  := include

# Executable name
TARGET   := ww

# Find all .c files recursively
SOURCES  := $(shell find $(SRCDIR) -type f -name '*.c')
OBJECTS  := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

# Default target
.PHONY: all
all: $(BINDIR)/$(TARGET)

# Create necessary directories
$(OBJDIR) $(BINDIR):
	mkdir -p $@

# Compile .c to .o
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link everything
$(BINDIR)/$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Debug build
.PHONY: debug
debug: CFLAGS := $(filter-out -O2,$(CFLAGS)) $(DEBUG_FLAGS)
debug: all

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Run the program
.PHONY: run
run: all
	./$(BINDIR)/$(TARGET)

# Run debug version
.PHONY: run-debug
run-debug: debug
	./$(BINDIR)/$(TARGET)

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all         - Build optimized release version (default)"
	@echo "  debug       - Build with -O0 and full debug symbols (-g3)"
	@echo "  run         - Build and run release version"
	@echo "  run-debug   - Build and run debug version"
	@echo "  clean       - Remove build artifacts"
	@echo "  help        - Show this help"

# Prevent make from deleting intermediate files
.PRECIOUS: $(OBJDIR)/%.o
