# NOTE: run premake.py to generate config.mk
-include config.mk

DOCDIR := ../docs

DEBUG_FLAGS := -O0 -g3

LDFLAGS :=

SRCDIR := .
OBJDIR := obj
BINDIR := bin
INCLUDE := include

TARGET := ww

# Find all .c files
SOURCES := $(shell find $(SRCDIR) -type f -name '*.c')
OBJECTS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

# Find all .org files in the parent directory
DOCSRC := $(wildcard ../*.org)
DOCTARGETS := $(patsubst ../%.org, $(DOCDIR)/%.html, $(DOCSRC))

# Check if emacs is available
EMACS := $(shell command -v emacs 2>/dev/null)

PREFIX ?= /usr/local
DESTDIR ?=

# Default target
.PHONY: all
all: $(BINDIR)/$(TARGET)

# Create directories
$(OBJDIR) $(BINDIR):
	mkdir -p $@

# Compile .c to .o
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link everything
$(BINDIR)/$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.PHONY: debug
debug: CFLAGS := $(filter-out -O2,$(CFLAGS)) $(DEBUG_FLAGS)
debug: all

.PHONY: install
install: all
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m 755 "$(BINDIR)/$(TARGET)" "$(DESTDIR)$(PREFIX)/bin/$(TARGET)"

.PHONY: uninstall
uninstall:
	-rm -f "$(DESTDIR)$(PREFIX)/bin/$(TARGET)"

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: distclean
distclean:
	rm -f config.mk
	rm -f include/config.h
	rm -f ../*.html

.PHONY: run
run: all
	./$(BINDIR)/$(TARGET)

.PHONY: run-debug
run-debug: debug
	./$(BINDIR)/$(TARGET)

.PHONY: docs
docs:
	@if [ -z "$(EMACS)" ]; then \
		echo "Error: emacs is not installed or not in PATH."; \
		echo "Please install emacs to build the documentation."; \
		exit 1; \
	fi
	@if [ -z "$(DOCSRC)" ]; then \
		echo "Warning: No .org files found in ../"; \
		exit 0; \
	fi
	@echo "Generating documentation for $(words $(DOCSRC)) Org file(s)..."
	@for orgfile in $(DOCSRC); do \
		htmlname=$$(basename "$$orgfile" .org).html; \
		echo "  Processing: $$orgfile to $(DOCDIR)/$$htmlname"; \
		( emacs --batch \
			--eval "(require 'org)" \
			--eval "(setq org-confirm-babel-evaluate nil)" \
			--eval "(find-file \"$$orgfile\")" \
			--eval "(org-html-export-to-html nil nil nil t)" \
			--eval "(kill-buffer)" \
			2>&1 | tail -n +2 ); \
	done
	@echo "Documentation successfully generated"

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all         - Build optimized release version (default)"
	@echo "  debug       - Build with -O0 and full debug symbols (-g3)"
	@echo "  docs        - Generate HTML from all ../*.org files using emacs"
	@echo "  run         - Build and run release version"
	@echo "  run-debug   - Build and run debug version"
	@echo "  clean       - Remove build artifacts"
	@echo "  distclean   - Remove premake artifacts and docs directory"
	@echo "  help        - Show this help"

# Prevent deleting intermediate files
.PRECIOUS: $(OBJDIR)/%.o
