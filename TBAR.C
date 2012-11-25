#include <stdio.h>
#include <stdlib.h>

typedef struct _TBAR
{
 unsigned size;
 unsigned long max; /* Per C 16-bit le cose cambiano... */
 unsigned long pct;
 char bar[1];
} TEXTBAR;

TEXTBAR* tbar_init(unsigned size)
{
 TEXTBAR* p = (TEXTBAR*) malloc( sizeof(TEXTBAR) + size );
 p->size = size;
 memset( p->bar,0,size+1 );
 memset( p->bar,177,size );

 return p;
}

char* tbar_sprintf(TEXTBAR* tb, unsigned long i)
{
 unsigned d;

 tb->pct = ( (i*100/tb->max) <= 100? i*100/tb->max : 100 );
 d = tb->pct*tb->size/100;
 memset(tb->bar,219,d);
 memset(tb->bar+d,177,tb->size-d);

 return tb->bar;
}

void tbar_free(TEXTBAR* tb)
{
 char a[80], n=80;
 memset(a,0,n--);
 memset(a,' ',n);
 printf("%s\r",a);
 free(tb);
}

#ifdef TEST
/* Perch‚ il test fallisce con WATCOM 16-bit e non con BORLAND 16-bit??? */
/* R: bisognava includere stdio.h e stdlib.h... un compilatore MOLTO pretenzioso! */
int main(int argc, char** argv)
{
 TEXTBAR* b = tbar_init(24);
 long a;

 puts("Esempio di barra progressiva...");
 b->max = 3333;
 for ( a=0; a < b->max+100; a++ )
 {
  tbar_sprintf(b,a);
  printf("Percentuale: %s %d%%\r",b->bar,b->pct);
 }
 for ( a=3333; a > 0; a-- )
 {
  tbar_sprintf(b,a);
  printf("Percentuale: %s %d%%\r",b->bar,b->pct);
 }

 tbar_free(b);

 return 1;
} 
#endif
