@echo off
copy ORIGINALE IN
perl crack.pl IN -RC 5 >.log
perl crack.pl IN -RS 5 >>.log
perl crack.pl IN -RI 5 >>.log
