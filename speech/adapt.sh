#!/bin/sh

# $1 = user label

#for file in `dir -d ./adapt/audio/*` ; do
#file=`expr substr $file 15 $((${#file}))`
#file=`expr substr $file 1 $((${#file}-4))`

file=$1

echo "Adapting $file .."

echo "(1/4) Make recipe file .."

echo "audio=./adapt/audio/$file.wav lna=./adapt/lnas/$file.lna transcript=./adapt/phns/tags.phn alignment=./adapt/segs/$file.seg speaker=$file" > $HOME/picsom/speech/adapt/recipes/$file.recipe

echo "(2/4) Make alignment .."

./aku/align --beam=1000 --sbeam=100 --swins=4000 -b ./models/wsj0_ml_pronaligned_tied1_gain3500_occ200_20.2.2010_22 -c ./adapt/cfgs/wsj0_ml_pronaligned_tied1_gain3500_occ200_20.2.2010_22.cfg -r $HOME/picsom/speech/adapt/recipes/$file.recipe

echo "(3/4) Make speaker configure file .."

$HOME/picsom/speech/aku/mllr -b $HOME/picsom/speech/models/wsj0_ml_pronaligned_tied1_gain3500_occ200_20.2.2010_22 -c $HOME/picsom/speech/adapt/cfgs/wsj0_ml_pronaligned_tied1_gain3500_occ200_20.2.2010_22.cfg -r $HOME/picsom/speech/adapt/recipes/$file.recipe -O -M mllr -S $HOME/picsom/speech/adapt/spkcs/default_mllr.spkc -o $HOME/picsom/speech/adapt/spkcs/$file.spkc 

echo "(4/4) Merge speaker configure file into main configure file"

LINES=`head -n 14 $HOME/picsom/speech/adapt/spkcs/${file}.spkc | tail -n 2`
MATRIX=`head -n 13 $HOME/picsom/speech/adapt/spkcs/${file}.spkc | tail -n 1`
BIAS=`head -n 14 $HOME/picsom/speech/adapt/spkcs/${file}.spkc | tail -n 1`
#echo $MATRIX
#echo $BIAS
cat $HOME/picsom/speech/adapt/cfgs/template.cfg | sed "s/REPLACEMATRIX/$MATRIX/" > temp
cat temp | sed "s/REPLACEBIAS/$BIAS/" > $HOME/picsom/speech/adapt/cfgs/$file.cfg
rm temp 

echo "Done."

#done

