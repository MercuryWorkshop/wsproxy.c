CC=gcc
CFLAGS=-O3 -march=znver1

ODIR=obj
SDIR=src

LIBS=-lm

_OBJ = main.o sha1.o base64.o handshake.o util.o net.o ws.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(SDIR)/%.c 
	$(CC) -c -o $@ $< $(CFLAGS)

wsproxy: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o wsproxy
