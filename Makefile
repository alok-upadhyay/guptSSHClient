CC=gcc
CFLAGS=-I.
DEPS=ssh_client.h
OBJ=ssh_client.o
LIBS=-lssl -lcrypto

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

guptSSHClient : $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

.PHONY: clean

clean : 
	rm -f guptSSHClient $(OBJ) *.swp *.o *.swo
