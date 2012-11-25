/*8.8.2004: aggiunto il doppio punto ai parametri, poiché DJGPP non conserva la riga di comando come MSVC! */
#include "incl.h"

#ifdef PBAR
#include "tbar.c"
TEXTBAR* b;
#endif

/* Notifica un messaggio, assumendo un numero variabile di argomenti secondo il valore di err */
void notify(int err, ...)
{
 char *msg="Il file contiene %lu byte ";
 char *msg2=" del file dal settore %lu...\n";
 va_list li;
 va_start(li, err);

 if (err == EFIO_BAD_SECTOR) {
  ULONG n = va_arg(li,ULONG);
  printf("Settore %lu guasto.",n);
 } else if (err == EFIO_REPORT) {
  ULONG n = va_arg(li,ULONG);
  if (n)
   printf("\nIndividuati %lu settori guasti: esito come sopra.\n",n);
  else
   printf("\nIl file appare integro.\n");
 } else if (err == EFIO_REPAIR_SECTOR) {
  int e = va_arg(li,int);
  if (e == EFIO_ALL_OK)
   printf(".. riparato!\r");
  else if (e == EFIO_FORCED_REPAIR)
   printf(".. tentata la riparazione.\n");
  else if (e == EFIO_CANT_REPAIR)
   printf(".. irreparabile!\n");
 } else if (err == EFIO_XOR_INIT) {
#ifdef PBAR
  ULONG nsec = va_arg(li,ULONG);
  b = tbar_init(25);
  b->max = nsec;
#endif
 } else if (err == EFIO_XOR_STEP) {
#ifdef PBAR
  ULONG k = va_arg(li,ULONG);
  tbar_sprintf(b,k);
  printf("Completato: %s %2d%%\r",b->bar,b->pct);
#endif
 } else if (err == EFIO_MINOR) {
  ULONG n = va_arg(li,ULONG);
  printf(msg,n);
  printf("meno dell'originale.\n");
 } else if (err == EFIO_MAIOR) {
  ULONG n = va_arg(li,ULONG);
  printf(msg,n);
  printf("pi— dell'originale.\n");
 } else if (err == EFIO_REDUCING) {
  ULONG n = va_arg(li,ULONG);
  printf("Riduzione");
  printf(msg2,n);
 } else if (err == EFIO_INFLATING) {
  ULONG n = va_arg(li,ULONG);
  printf("Espansione");
  printf(msg2,n);
 }
 va_end(li);
}


int main(int argc, char** argv)
{
 FILE *in, *sec;
 HEADER h;
 char *sec_name=0, mode_repair=0, force_repair=0, pm, found=1;
 char *msg="Impossibile aprire il file '%s'.\n";
 long settori=0;
 double pct = 1;

 for (pm=1; pm<argc; pm++) {
  char opt;
  if (argv[pm][0] != '/') continue;
  if (argv[pm][1] == '?') {
   printf( "Protegge o ripara un file mediante un blocco di sicurezza.\n\n" \
"PROTEGE [/R] [/F] [/O:file] [/S:n] [/P:n] file\n\n" \
"  /R   analizza un file, tentandone la riparazione\n" \
"  /F   forza il recupero parziale di un settore guasto\n" \
"  /S   dimensione del blocco di sicurezza, in settori di 512 byte\n" \
"  /P   dimensione del blocco di sicurezza, in percentuale del file protetto\n" \
"       (normalmente, 1%; massimo: 10%)\n" \
"  /O   file contenente le informazioni di sicurezza\n" \
"       (in mancanza, aggiunge l'estensione .SEC al file da elaborare)");
   return 1;
  }
  opt = toupper(argv[pm][1]);
  if (opt== 'S') {
   settori = atol(&argv[pm][3]);
   found++;
   continue;
  }
  if (opt== 'O') {
   sec_name = &argv[pm][3];
   found++;
   continue;
  }
  if (opt== 'R') {
   mode_repair = 1;
   found++;
   continue;
  }
  if (opt== 'F') {
   force_repair = 1;
   found++;
   continue;
  }
  if (opt== 'P') {
   pct = strtod(&argv[pm][3],0);
   if (pct > 10) pct=10;
   if (pct <= 0) pct=1;
   found++;
   continue;
  }
  printf("Parametro non riconosciuto: '%s'.\n",argv[pm]);
  return 1;
 }

 argv+=found;
 argc-=found;

 if (argc < 1) {
  printf("Occorre indicare il file da proteggere.");
  return 1;
 }

 if (!sec_name) {
  sec_name = malloc(strlen(argv[0])+5);
  strcpy(sec_name,argv[0]); strcat(sec_name,".SEC");
 }

 if (mode_repair) {
  USHORT crc;
  in=fopen(argv[0],"r+b");
  sec=fopen(sec_name,"rb");
  if (!in) {
   printf(msg,argv[0]);
   return 1;
  }
  if (!sec) {
   printf(msg,sec_name);
   return 1;
  }
  fread(&h,sizeof(HEADER),1,sec);
  if (h.Sig != MAGIC) {
   printf("Il file di sicurezza non Š valido.");
   return 1;
  }
  crc = crc_file(sec);
  if (h.CRC != crc) {
   printf("Il blocco di sicurezza Š corrotto.\n");
   return 1;
  }
  repair_file(in,sec,h.nsec,h.size,force_repair,notify);
  goto chiudi;
 } else { /* mode_repair == 0 */
  in=fopen(argv[0],"rb");
  if (!in) {
   printf(msg,argv[0]);
   return 1;
  }
  sec=fopen(sec_name,"w+b"); /*10.8.2004: meglio creare il file certi la sorgente esiste*/
  if (!sec) {
   printf(msg,sec_name);
   return 1;
  }
  fseek(in,0,SEEK_END);
  if (settori==0) settori = (long) (ftell(in)/100*pct)/512;
  if (settori==0) settori = 1; /* extrema ratio... */
  if (settori*SECTOR > ftell(in)) {
   printf("Il blocco Š stato ridotto poich‚ superava il file da proteggere.\n");
   settori = (long) ftell(in)/512;
  }
  if (settori==0) settori = 1; /*10.8.2004:qui può ancora essere zero, se il file è inferiore al serttore!*/
  printf("Creazione di un file di protezione da %lu settori...\n",settori);
  init_header(&h,settori,ftell(in),0);
  fwrite(&h,sizeof(HEADER),1,sec);
  /* Calcola il CRC-16 di ciascun settore dell'input */
  rewind(in);
  file_crc(in,sec);
  /* Calcola lo XOR a blocchi composti dal numero di settori dato */
  rewind(in);
  file_xor(in,sec,settori,notify);
  /* Calcola e registra il CRC-16 dei dati di sicurezza */
  rewind(sec);
  fseek(sec,sizeof(HEADER),SEEK_SET);
  h.CRC = crc_file(sec);
  rewind(sec);
  fwrite(&h,sizeof(HEADER),1,sec);
 }
chiudi:
 fclose(in);
 fclose(sec);
 return 0;
}


void init_header(HEADER* h, ULONG nsec, ULONG size, USHORT crc)
{
 h->Sig = MAGIC;
 h->nsec = nsec;
 h->size = size;
 h->CRC = crc;
}
