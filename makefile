IDIR = include
CC = gcc
CFLAGS = -Wall -I$(IDIR) -I/usr/local/opt/openssl/include

ODIR = obj
LDIR = lib

LIBS = -L /usr/local/opt/openssl/lib -lcrypto -lssl

_DEPS = hashmake.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = hashmake.o hashfunc.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(OBJ): $(ODIR)/%.o: src/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

hash: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 