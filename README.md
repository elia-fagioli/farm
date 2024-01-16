# farm

Corso: Sistemi Operativi.

# Descrizione

L'implementazione del progetto comprende diversi aspetti, tra cui i fondamenti di programmazione in C, la gestione di strumenti di debug come GDB e valgrind, la realizzazione di un Makefile, la corretta gestione di path e directory, chiamate di sistema, gestione di thread POSIX e processi, comunicazione socket (in locale), gestione dei segnali con conseguente realizzazione di maschere e signal handler, e l'utilizzo di script bash per il testing e operazioni varie.

# Makefile – Comandi

Il Makefile fornisce diversi comandi utili per la compilazione, il testing e la pulizia del sistema:

make all: Compila e collega i target farm, generafile e collector.

make test: Esegue il comando "bash test.sh" per avviare il testing con lo script 'test.sh'

make clean: Pulisce il sistema dai file generati durante la compilazione e il testing.

# Generatore di file binari

generafile.c: Codice per la generazione di file binari contenenti valori long (usato per il testing).

# Ulteriori testcase

Presenti all'interno della directory testcases




