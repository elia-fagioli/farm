#!/bin/bash

# Controlla se gli eseguibili sono presenti
if [ ! -e generafile ]; then
    echo "Compilare generafile, eseguibile mancante"
    exit 1
fi

if [ ! -e farm ]; then
    echo "Compilare farm, eseguibile mancante"
    exit 1
fi

# Genera i file di input con il programma "generafile"
for i in {1..32}; do
    ./generafile file$i.dat $(($i*1000)) > /dev/null
done

# Esecuzione con 32 thread
./farm -n 32 file*.dat > output.txt

# Verifica se l'output coincide con l'output atteso
cat > expected_output.txt <<EOF
44078144 file1.dat
172530018 file2.dat
385471347 file3.dat
696606187 file4.dat
1081707668 file5.dat
1553675073 file6.dat
2115674547 file7.dat
2763504909 file8.dat
3488056257 file9.dat
4295636149 file10.dat
5196410209 file11.dat
6190574785 file12.dat
7264667117 file13.dat
8458174691 file14.dat
9728036499 file15.dat
11068585127 file16.dat
12425419688 file17.dat
13905520999 file18.dat
15449517042 file19.dat
17134475499 file20.dat
18869509716 file21.dat
20725454593 file22.dat
22687991498 file23.dat
24755434855 file24.dat
26866375823 file25.dat
29117168898 file26.dat
31349307720 file27.dat
33689673006 file28.dat
36171622779 file29.dat
38777056723 file30.dat
41341313397 file31.dat
44134248156 file32.dat
EOF

diff output.txt expected_output.txt
if [[ $? == 0 ]]; then
    echo "Test con 32 threads superato"
else
    echo "Test con 32 threads fallito"
fi

# Pulizia
rm -r file* expected_output.txt output.txt

