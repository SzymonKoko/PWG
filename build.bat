@echo off
echo Tworzenie katalogu build...
if not exist build mkdir build
cd build

echo Generowanie solucji Visual Studio 2022...
cmake -G "Visual Studio 17 2022" -A x64 ..

echo Gotowe! Teraz mozesz otworzyc plik PWG.sln w Visual Studio 2022.
pause
