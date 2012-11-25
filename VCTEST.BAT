@echo off
copy ORIGINALE IN
perl crack.pl IN -RC 10 >.log
perl crack.pl IN -RS 10 >>.log
perl crack.pl IN -RI 10 >>.log
