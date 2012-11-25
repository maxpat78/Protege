rem call \djgpp\djset
call \gw
@echo off
rem Versione release (con barra di avanzamento)
gcc -DPBAR -DUSE_CRC32 -w -O3 -march=i586 main.c AFIO.c -o PROTEGE.exe
rem Versione release per test (senza barra di avanzamento)
gcc -DUSE_CRC32 -Wall -O3 -march=i586 main.c AFIO.c -o repair.exe
upx -9 *.exe
