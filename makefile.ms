O=.obj
CC=cl -nologo
CFLAGS=-MD -O2 -G5Af -DPBAR -DUSE_CRC32
OBJS=main$O AFIO$O

REPAIR.EXE: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) /link /out:REPAIR.EXE

main$O: main.c incl.h makefile.ms
afio$O: AFIO.c incl.h makefile.ms

clean:
	del *$O
	del repair.exe
