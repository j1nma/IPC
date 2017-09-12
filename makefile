TARGET = hash

CC = gcc
CFLAGS = -Wall -I$(IDIR) -I/usr/local/opt/openssl/include

LINKER = gcc
LFLAGS = -L /usr/local/opt/openssl/lib -lcrypto -lssl -lrt -lpthread

SRCDIR = src
OBJDIR = obj
BINDIR = bin
IDIR = $(SRCDIR)/include

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(IDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm       = rm -f

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"