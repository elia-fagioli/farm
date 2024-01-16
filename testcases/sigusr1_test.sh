#!/bin/bash

# Esegui farm con delay
./farm -n 1 -q 1 -t 2000 file* -d testdir &
pid=$!

# Aspetta 5 secondi e poi invia segnale SIGUSR1 al processo farm
sleep 5
kill -USR1 $pid
sleep 1
kill -USR1 $pid
sleep 1
kill -USR1 $pid
# Aspetta 3 secondi e invia SIGTERM al processo farm
sleep 3
kill $pid

# Attendi che farm termini
wait $pid

echo "Test SIGUSR1 completato"

