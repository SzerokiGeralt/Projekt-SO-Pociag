#!/bin/bash

# Nazwy plików źródłowych i docelowych
FILES=("master" "zawiadowca" "kierownik" "pasazer")

# Kompilacja każdego pliku
for FILE in "${FILES[@]}"; do
    echo "Kompiluje $FILE.c..."
    gcc -o $FILE $FILE.c -lrt -lpthread
    if [ $? -eq 0 ]; then
        echo "$FILE skompilowany pomyslnie."
    else
        echo "Blad podczas kompilacji $FILE."
        exit 1
    fi
done

echo "Wszystkie pliki zostaly skompilowane pomyslnie!"
