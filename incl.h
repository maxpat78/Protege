#if !defined(MY_INCL_H_INCLUDED)
#define MY_INCL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define MAGIC 0x05045242
#define SECTOR 512
#define ALIGN 8

typedef unsigned long ULONG;
typedef unsigned short USHORT;

#pragma pack(1)
typedef struct _HEADER
{
 unsigned Sig; /* MAGIC 0x42520405 */
 unsigned short CRC; /* CRC16 del file di sicurezza, dal prossimo byte */
 ULONG nsec; /* Settori Per un Blocco */
 ULONG size; /* Dimensione del file protetto */
} HEADER;
#pragma pack()

enum {
 EFIO_READ_SECTOR,
 EFIO_WRITE_SECTOR,
 EFIO_READ_CRC,
 EFIO_ERRORS_DETECTED,
 EFIO_TOO_MANY_ERRORS,
 EFIO_ALL_OK,
 EFIO_CANT_REPAIR,
 EFIO_FORCED_REPAIR,
 EFIO_BAD_SECTOR,
 EFIO_REPAIR_SECTOR,
 EFIO_REPORT,
 EFIO_XOR_INIT,
 EFIO_XOR_STEP,
 EFIO_MINOR,
 EFIO_MAIOR,
 EFIO_TMP_FILE,
 EFIO_REDUCING,
 EFIO_INFLATING,
};

/* Puntatore a una funzione di notifica con argomenti variabili */
typedef void (*NOTIFY)(int, ...);

void init_header(HEADER* h, ULONG nsec, ULONG size, USHORT crc);
int repair_sector(FILE* in, FILE* sec, ULONG nsec, ULONG sector, char force);
int repair_file(FILE* in, FILE* sec, ULONG nsec, ULONG realsize, char force, NOTIFY say);

USHORT crc16_for_block(USHORT crc, unsigned char *data, int len);
int file_crc(FILE* in, FILE* out);
USHORT crc_file(FILE* in);
int file_xor(FILE* in, FILE* out, ULONG nsec, NOTIFY);
ULONG filesize(FILE* f);
void insert(FILE *f, ULONG pos, ULONG len);
void shrink(FILE *f, ULONG pos, ULONG len);


#ifdef __cplusplus
}
#endif

#endif /* MY_INCL_H_INCLUDED */
