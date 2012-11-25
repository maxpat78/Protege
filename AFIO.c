/*
 AFIO.C - Funzioni di I/O ANSI per l'utilità di protezione e recupero dei dati.
*/

/*
"SICUREZZA A BLOCCHI" (WinACE & WinRAR docent!)
31.12.2002 - ...

TEORIA:
è possibile ricavare da ogni file un "blocco di sicurezza", tale da permettere
la riparazione di guasti, purché il danno presenti dati caratteri.

PRASSI:
Per proteggere un file:
a) si divide il file in SETTORI di uguale dimensione (pe. 512 byte);
b) si calcola e si conserva il CRC di ciascun settore;
c) si fissa la dimensione del BLOCCO di sicurezza a un certo numero 'n' di settori;
indi si calcola lo XOR ("somma posizionale") di tutti i settori equidistanti 'n' settori,
dal primo settore del file all'ennesimo.

Per verificare e riparare il file:
d) si confrontano i CRC16 dei settori;
e) individuato un settore guasto, si calcola lo XOR di tutti i settori equidistanti,
eccettuato quello guasto;
f) infine, si aggiunge il corrispondente settore di sicurezza: il settore così
ottenuto riproduce il settore guasto.

NOTE:
1) LIMITI DI RIPARABILITA'
Quando più settori equidistanti sono guasti, la riparazione completa è impossibile, e solamente
singoli byte potranno essere recuperati.

IN PRATICA (proteggendo con 64 settori da 512 byte un file):
- guasto dei settori 0-63 (i primi 32768 byte): tutti i dati possono essere recuperati;
- guasto dei byte 1-32768 (settori 0-64): è irrecuperabile il primo byte dei settori 1 e 64,
poiché lo XOR dei settori 64, 128... risulterà difettoso nel primo byte, e così pure quello
dei settori 0, 128...;in teoria, il guasto dovrebbe poter essere riparato, poiché i byte
guasti sono pur sempre 32768: ma, dato che il programma riconosce solo i guasti di interi
settori e non di singoli byte, esso ignora le esatte posizione ed estensione del guasto;
- un singolo byte, in definitiva, può essere riparato purchè tutti gli altri byte equidistanti
siano sani.

2) FILE DI DIVERSA DIMENSIONE
L'applicazione ripara detti file solo se l'intero guasto (ammanco o inserzione di byte) è
concentrato in un'unica posizione.
Una volta individuato il primo settore guasto, il programma sposta la successiva parte
del file guasto avanti (se il file è stato ridotto) o indietro (se il file è stato espanso),
quindi, raggiunta la dimensione originale, applica l'usuale tecnica di riparazione. 

3) BUGS
Si è presentato un caso nel quale l'applicazione ha omesso la riparazione di almeno un settore
reputandolo intatto quando non lo era: ciò fa pensare che possano capitare CRC-16 identici per
settori diversi.
*/

#include "incl.h"

#ifndef USE_CRC32
 #include "crc16.c"
#else
 #define DYNAMIC_CRC_TABLE
 #define crc16_for_block(crc,data,len) (USHORT) crc32(crc,data,len)
 #include "crc32.c"
#endif

/* Esegue lo XOR di due blocchi src e dst per size byte, ponendo il risultato in dst */
__inline char* memxor(char* dst, char* src, unsigned size)
{
 while (size >= ALIGN)
 {
  *((int*)dst)++ ^= *((int*)src)++;
  *((int*)dst)++ ^= *((int*)src)++;
  size -= ALIGN;
 }
 if (size) do {
  *dst++ ^= *src++;
 } while (--size);
 return dst;
}


/*
 Ripara un singolo settore:
 in		file guasto
 sec		file contenente il blocco di sicurezza
 nsec		numero di settori componenti il blocco di sicurezza
 sector	settore guasto
 force		se rigenerare anche un settore recuperabile solo in parte
*/
int repair_sector(FILE* in, FILE* sec, ULONG nsec, ULONG sector, char force)
{
 int err = EFIO_ALL_OK;
 char xorblock[SECTOR];
 ULONG pos, i = sector%nsec; /* Posizione del settore guasto relativa al blocco di appartenenza */
 USHORT crc, crc2;
 long size, start=sector*SECTOR;

 pos = ftell(in);
 size = filesize(in);

 memset(xorblock,0,SECTOR);
 /* Prepara la lettura del primo settore */
 fseek(in,(long)i*SECTOR,SEEK_SET);

 for (;;)
 {
  char buf[SECTOR];
  size_t read;
/* Ignora il settore da riparare */
  if ( ftell(in) == start )
  {
   fseek(in,(long)nsec*SECTOR,SEEK_CUR);
   fseek(sec,(long)nsec*sizeof(USHORT),SEEK_CUR);
   continue;
  }
/* Aggiunge il corrispondente settore dal blocco di sicurezza */
/* dopo aver elaborato i settori sani */
  if (size <= ftell(in))
  {
   fseek(sec,0,SEEK_END);
   fseek(sec,(long)-(nsec-i)*SECTOR,SEEK_CUR);
   read = fread(buf,1,SECTOR,sec);
   memxor(xorblock,buf,read);
   break;
  }
  read = fread(buf,1,SECTOR,in);
  if (ferror(in)) return EFIO_READ_SECTOR;
  memxor(xorblock,buf,read);
  fseek(in,(long)(nsec-1)*SECTOR,SEEK_CUR);
 }
 /* Determina se la riparazione è possibile, confrontando i CRC del settore originale e di quello rigenerato */
 crc = crc16_for_block(0,xorblock,(start+SECTOR > size)? size-start : SECTOR);
 fseek(sec,(long)sizeof(HEADER)+sector*sizeof(USHORT),SEEK_SET);
 fread(&crc2,sizeof(USHORT),1,sec);
 if (crc!=crc2) err = EFIO_CANT_REPAIR;
 if (err==EFIO_CANT_REPAIR && !force) goto fine;
 if (err==EFIO_CANT_REPAIR && force) err=EFIO_FORCED_REPAIR;
 fseek(in,start,SEEK_SET);
 /* Se il settore finale è incompleto (piuttosto comune!), scrive solo la parte mancante */
 fwrite(xorblock,(start+SECTOR > size)? size-start : SECTOR,1,in);
 if (ferror(in)) return EFIO_WRITE_SECTOR;
 /* Riporta il puntatore dove repair_file l'aveva lasciato... */
fine:
 fseek(in,pos,SEEK_SET);
 return err;
}


/*
 Calcola il CRC16 di tutti i settori del file da proteggere
 in	file da proteggere
 out	file sul quale scrivere i CRC16
*/
int file_crc(FILE* in, FILE* out)
{
 char buf[SECTOR];
 size_t read;

 while (feof(in) == 0)
 { 
  USHORT crc; 
  read = fread(buf,1,SECTOR,in);
  if (ferror(in))
  {
   return EFIO_READ_SECTOR;
  }
  crc = crc16_for_block(0,buf,read);
  fwrite(&crc,sizeof(USHORT),1,out);
  if (ferror(out))
  {
   return EFIO_WRITE_SECTOR;
  }
 }
 return EFIO_ALL_OK;
}


/*
 Calcola il CRC16 di un file
*/
USHORT crc_file(FILE* in)
{
 USHORT crc=0; 
 char buf[SECTOR];
 size_t read;

 while (feof(in) == 0)
 { 
  read = fread(buf,1,SECTOR,in);
  if (ferror(in))
  {
   return EFIO_READ_SECTOR;
  }
  crc = crc16_for_block(crc,buf,read);
 }
 return crc;
}


/*
 Calcola lo XOR dei blocchi del file da proteggere
 in	file da proteggere
 out	file di protezione
 nsec	dimensione del blocco, in settori
*/
int file_xor(FILE* in, FILE* out, ULONG nsec, NOTIFY say)
{
 ULONG k, size;

 size = filesize(in);

 say(EFIO_XOR_INIT,nsec);

 for (k=0; k < nsec; k++)
 {
  char xorblock[SECTOR];
  memset(xorblock,0,SECTOR);
  fseek(in,(long)k*SECTOR,SEEK_SET);
  for (;;)
  {
   char buf[SECTOR];
   size_t read;
   read = fread(buf,1,SECTOR,in);
   if (ferror(in))
   {
    return EFIO_READ_SECTOR;
   }
   memxor(xorblock,buf,read);
   fseek(in,(long)(nsec-1)*SECTOR,SEEK_CUR);
   if (size <= ftell(in)) break;
  }
  fwrite(xorblock,SECTOR,1,out);
  if (ferror(out))
  {
    return EFIO_WRITE_SECTOR;
  }
  say(EFIO_XOR_STEP,k);
 }
 say(EFIO_XOR_STEP,k);
 return EFIO_ALL_OK;
}


ULONG filesize(FILE* f)
{
 ULONG size, pos;

 pos = ftell(f);
 fseek(f,0L,SEEK_END);
 size = ftell(f);
 fseek(f,pos,SEEK_SET);
 return size;
}


/* Inserisce len byte nel file f dalla posizione pos (=sposta avanti una parte del file); o aggiunge, se pos==end */
void insert(FILE *f, ULONG pos, ULONG len)
{
#define BLOCK 65535
 char buf[BLOCK];
 ULONG start, end, tomove, blks;

 end = filesize(f);
 tomove = end-pos; /* byte da spostare */
 start = tomove>BLOCK? end-BLOCK : end-tomove; /* posizione iniziale del blocco da spostare */

 if (start==end) goto dontmove;

 for (blks=tomove/BLOCK; blks; start-=BLOCK,blks--)
 {
  fseek(f,start,SEEK_SET);
  fread(buf,1,BLOCK,f);
  fseek(f,start+len,SEEK_SET);
  fwrite(buf,1,BLOCK,f);
 }
 /* Sposta gli ultimi byte */
 tomove %= BLOCK;
 fseek(f,pos,SEEK_SET);
 fread(buf,1,tomove,f);
 fseek(f,pos+len,SEEK_SET);
 fwrite(buf,1,tomove,f);
 return;

dontmove:
 /* Inserisce alla fine del file */
 fseek(f,start+len-1,SEEK_SET);
 fwrite(buf,1,1,f);
}


/* Riduce il file f di len byte dalla posizione pos */
void shrink(FILE *f, ULONG pos, ULONG len)
{
#define BLOCK 65535
 char buf[BLOCK];
 ULONG start, end, tomove, blks;

 end = filesize(f);
 tomove = end-pos-len; /* byte da spostare */
 start = pos+len; /* posizione iniziale del blocco da spostare */

 for (blks=tomove/BLOCK; blks; start+=BLOCK,blks--)
 {
  fseek(f,start,SEEK_SET);
  fread(buf,1,BLOCK,f);
  fseek(f,start-len,SEEK_SET);
  fwrite(buf,1,BLOCK,f);
 }
 /* Sposta gli ultimi byte */
 tomove %= BLOCK;
 fseek(f,(long)-tomove,SEEK_END);
 fread(buf,1,tomove,f);
 fseek(f,(long)-(tomove+len),SEEK_END);
 fwrite(buf,1,tomove,f);
/* Come si imposta la fine di un file in modo ANSIsh??? */
 chsize(fileno(f),end-len);
}


/*
 Cerca i guasti di un file e tenta di ripararli
 in		file da verificare o riparare
 sec		file di sicurezza
 nsec		settori del blocco
 realsize 	dimensione originaria
 force		se eseguire la riparazione parziale
 say		funzione di notifica a parametri variabili
*/
int repair_file(FILE* in, FILE* sec, ULONG nsec, ULONG realsize, char force, NOTIFY say)
{
 char buf[SECTOR];
 ULONG size, delta, seen;
restart:
 seen = delta = 0;
 size = filesize(in);
 rewind(in);

 if (size < realsize) {
  delta = realsize-size;
  say(EFIO_MINOR,delta);
 }
 if (size > realsize) {
  delta = size-realsize;
  say(EFIO_MAIOR,delta);
 }

 while (ftell(in) < size) {
  ULONG n = ftell(in)/SECTOR;
  USHORT crc, crc2;
  size_t read;

  read = fread(buf,1,SECTOR,in);
  if (ferror(in)) return EFIO_READ_SECTOR;
  crc = crc16_for_block(0,buf,read);
  fseek(sec,sizeof(HEADER)+n*sizeof(USHORT),SEEK_SET);
  fread(&crc2,sizeof(USHORT),1,sec);
  if (ferror(sec)) return EFIO_READ_CRC;
  if (crc != crc2)  {
   if (delta) {
    if (size < realsize) {
     insert(in,n*SECTOR,delta);
     say(EFIO_INFLATING,n);
    } else {
     shrink(in,n*SECTOR,delta);
     say(EFIO_REDUCING,n);
    }
    goto restart;
   }
   say(EFIO_BAD_SECTOR,n);
   seen++;
   say(EFIO_REPAIR_SECTOR,repair_sector(in,sec,nsec,n,force));
  }
 }
 /* Se il guasto è alla fine... */
 if (delta) {
  insert(in,size,delta);
  say(EFIO_INFLATING,size/SECTOR+1);
  goto restart;
 }
 say(EFIO_REPORT,seen);
 return seen? EFIO_ERRORS_DETECTED : EFIO_ALL_OK;
}
