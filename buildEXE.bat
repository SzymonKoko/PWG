@echo off
echo Tworzenie katalogu build...
if not exist build mkdir build
cd build

echo Building game target...
cmake --build . --config Release --target PWG

echo Gotowe! 
