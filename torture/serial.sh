#!/bin/bash -e

INDIR=$1
OUTDIR=$2

for i in $INDIR/*.xz
do
	o=$OUTDIR/`basename  $i .xz`
	echo $i "->" $o
	time xzcat $i > $o
done
