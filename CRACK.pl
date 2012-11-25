use File::Copy;
use File::Compare;
$Z = 0;

sub crack {
 if ($#_ < 2) {die "usa: crack <file>, <pos>, <len>\n"};
 my ($f,$pos,$len) = @_;
 open F, "+<$f" or die "impossibile corrompere $f\n";
 seek F, $pos, 0;
 if ($pos+$len > $Z) {$len=$Z-$pos;}
 print F 'x' x $len;
 close F;
 print "Corrotto file $f per $len byte dal byte $pos.\n";
}

sub rcrack {
 if ($#_ < 1) {die "usa: rcrack <file>, <maxbytes>\n"};
 $P = int rand $Z;
 $L = int rand $_[1];
 crack $_[0], $P, $L;
}

sub shrink {
 if ($#_ < 2) {die "usa: shrink <file>, <pos>, <len>\n"};
 my ($f,$pos,$len) = @_;
 open F, "<$f"; open T, "+<$f" || die "Impossibile ridurre $f\n";
 binmode T; binmode F;
 if ($pos+$len > $Z) {$len=$Z-$pos;}
 seek F, $pos+$len, 0; seek T, $pos, 0;
 copy(\*F,\*T);
 truncate T, $Z-$len;
 close F; close T;
 print "Rimossi $len byte dalla posizione $pos.\n";
}

sub rshrink {
 if ($#_ < 1) {die "usa: rshrink <file>, <maxbytes>\n"};
 $P = int rand $Z;
 $L = int rand $_[1];
 shrink $_[0], $P, $L;
}


sub rndcrack {
 if ($#_ < 1) {die "usa: rcrack <file>, <bytes>\n"};
 $todo = $_[1];
 while ($todo > 0)
 {
  $P = int rand $Z;
  $L = int rand 512;
  crack $_[0], $P, $L;
  $todo -= $L;
 }
}


sub insert {
 if ($#_ < 2) {die "usa: insert <file>, <pos>, <len>\n"};
 my ($f,$pos,$len) = @_;
 open F, "<$f"; open T, "+<$f" || die "Impossibile gonfiare $f\n";
 binmode T; binmode F;
 $BLOCK=65536;
 $q = $Z-$pos;
 $fromp=$q>$BLOCK? $Z-$BLOCK : $Z-$q;
 for ($blks = int $q/$BLOCK; $blks; $fromp-=$BLOCK, $blks--) {
  seek F,$fromp,0;
  seek T, $fromp+$len, 0;
  read F,$buf,$BLOCK;
  print T $buf;
 }
 seek F, $pos, 0;
 seek T, $pos+$len, 0;
 read F,$buf,$q%$BLOCK;
 print T $buf;

 seek T,$pos,0;
 print T 'x' x $len;
 close F; close T;
 print "Inseriti $len byte dalla posizione $pos.\n";
}

sub rinsert {
 if ($#_ < 1) {die "usa: rinsert <file>, <maxbytes>\n"};
 $P = int rand $Z;
 $L = int rand $_[1];
 insert $_[0], $P, $L;
}


##
# INIZIO DELLA FESTA...
##
die "Usa: CRACK.pl <file> <-RC|-RS|-RI> <passaggi> || <file> [-C|-S|-I] <pos> <len>\n" if ($#ARGV < 1);

copy $ARGV[0],"$ARGV[0].bak";
print "Salvata copia del file $ARGV[0].\n";
$Z = ((stat($ARGV[0]))[7]);

$crackmode = 1;
$file = shift @ARGV; $op = shift @ARGV;
if    ($op eq '-RC') { $crackmode=0; }
elsif ($op eq '-RS') { $crackmode=1; }
elsif ($op eq '-RI') { $crackmode=2; }
elsif ($op eq '-C') { crack $file, $ARGV[0], $ARGV[1]; exit; }
elsif ($op eq '-S') { shrink $file,$ARGV[0], $ARGV[1]; exit; }
elsif ($op eq '-I') { insert $file,$ARGV[0], $ARGV[1]; exit; }

for (1..$ARGV[0]) {
 print "\n\nINIZIO DEL PASSAGGIO $_.\n";
 $s = 2 ** int rand 11;
 $cmd = sprintf "repair /S:%s %s", $s, $file;
 print "Protezione per $s settori.\n";
 print `$cmd`;
# $p = $s*512; # Il guasto PUO' non essere riparato se tocca un settore in piu'
 $p = ($s-1)*512; # Il guasto DEVE essere sempre riparato
 if ($crackmode==0) {rcrack $file,$p;}
 elsif ($crackmode==1) {rshrink $file,$p;}
 else {rinsert $file,$p;}
 $cmd = sprintf "repair /F /R %s", $file;
 for (`$cmd`) {print if not (/Settore [0-9]+ guasto... riparato!/);}
 if ( compare($file,"$file.bak")!=0 ) {
  print "ERRORE [$_]: file NON riparato!\n";
  copy "$file.bak",$file;
 }
}
__END__
Corrompe e ripara ripetutamente un file, verificando la riparazione.

 <file>	file da corrompere (salva sempre una copia <file>.BAK)
 -RC		corrompe un ammontare casuale di byte
 -RS		riduce di un numero casuale di byte, da una posizione a caso
 -RI		espande di un numero casuale di byte, da una posizione a caso
 <passaggi>numero di ripetizioni dell'operazione


		OPPURE


Distrugge, rimuove o inserisce un ammontare casuale o definito di byte da una
posizione casuale o prestabilita di un dato file.


 <file>	file da corrompere (salva sempre una copia <file>.BAK)
 -C		corrompe
 -S		riduce
 -RI		espande
 <pos>	posizione iniziale del guasto
 <len>	lunghezza del guasto

		!!! !!! !!! A T T E N Z I O N E !!! !!! !!!

Eseguendo una serie continua di test su un file di dimensioni "non piccole" sotto Windows NT4 SP4 (NTFS), può
accadere di trovare nel rapporto che qualche confronto è fallito: tuttavia, rieseguendo individualmente il
test dichiarato fallito, si trova che questo invece riesce.

Ciò sembrerebbe indicare qualche oscuro bug di NTFS o del kernel di NT: si ha l'impressione che, al momento
del confronto, l'aggiornamento di qualche parte del file non sia ancora completata come dovrebbe, ipotesi
particolarmente ragionevole se NTFS fosse davvero un FS "journaled".
