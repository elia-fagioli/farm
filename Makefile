# compilatore
CC = gcc

# flags compilatore (debug, warning, threading, inclusione delle directory)
CFLAGS = -g -Wall -Wextra -pthread -I./includes

# flag opzione linker (threading)
LFLAGS = -pthread

# directory sorgenti
SRCDIR = src

# directory degli oggetti compilati
OBJDIR = obj

# elenco file sorgente
SOURCES = $(wildcard $(SRCDIR)/*.c)

# dalle sorgenti otteniamo nomi oggetti compilati
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

# Targets
all: farm generafile collector

# regole dipendenze compilazione

farm: $(OBJDIR)/masterWorker.o $(OBJDIR)/worker_thread.o
	$(CC) $(LFLAGS) $^ -o $@

collector: $(OBJDIR)/collector.o
	$(CC) $(LFLAGS) $^ -o $@

generafile: $(OBJDIR)/generafile.o
	$(CC) $(LFLAGS) $< -o $@

# compilare un oggetto da una singola sorgente .c
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# target per test.sh
test: all
	bash test.sh

# pulizia file generati
clean:
	rm -f $(OBJDIR)/*.o farm generafile collector masterWorker expected.txt socket.sck file*
	rm -rf testdir

