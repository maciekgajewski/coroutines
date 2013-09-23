#!/bin/bash -e

INDIR=$1
OUTDIR=$2

for i in $INDIR/*.xz
do
	o=$OUTDIR/`basename -s .xz $i`
	echo $i "->" $o
	xzcat $i > $o
done
