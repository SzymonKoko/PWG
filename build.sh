#!/bin/bash
echo "Tworzenie katalogu build..."
mkdir -p build
cd build

echo "Generowanie plików Makefile..."
cmake ..

echo "Budowanie projektu..."
cmake --build .

echo "Gotowe!"
