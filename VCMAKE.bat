@echo off
rem Versione release (con barra di avanzamento)
cl -DPBAR -nologo -Oxb2 -MD -G5 -DUSE_CRC32 main.c AFIO.c /link /OUT:PROTEGE.exe

rem Versione release per test (senza barra di avanzamento)
cl -nologo -Ox -ML -G5 -DUSE_CRC32 main.c AFIO.c /link /OUT:REPAIR.exe

rem Versione debug
rem cl -nologo -ZI main.c AFIO.c -DUSE_CRC32 /link /OUT:REPAIR.exe /debug

del *.obj
