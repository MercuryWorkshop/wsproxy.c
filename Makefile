CC=gcc
CFLAGS=-g -fPIC

ODIR=obj
SDIR=src

LIBS=-lm

_OBJ = main.o sha1.o base64.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(SDIR)/%.c 
	$(CC) -c -o $@ $< $(CFLAGS)

webtcp: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o webtcp
