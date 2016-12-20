#!/bin/sh

cd $HOME/picsom/speech

## ================================
## LANGUAGE MODELS, LEXICON, ACOUSTIC MODELS
## ================================

if [ "$1" != "eng" ]; then ## FINNISH (default)

  LM=./models/morph40000_6gram_gt_1e-8.bin 
  LA_LM=./models/morph40000_2gram_gt.bin ## LOOK AHEAD 
  LEX=./models/morph40000.lex
  HMM=./models/speecon_mfcc_gain3500_occ225_mpe_19kmorphs_1gram_extviterbi_27.11.2007_4

else ## ENGLISH 
   
  #LM=./models/eng-gigaword-200M-60k-3gram.bin
  #LA_LM=./models/eng-gigaword-200M-60k-2gram.bin
  #LEX=./models/eng-gigaword-200M-60k.lex
  HMMBASE=wsj0_ml_pronaligned_tied1_gain3500_occ200_20.2.2010_22
  HMM=./models/wsj0_ml_pronaligned_tied1_gain3500_occ200_20.2.2010_22
  LM=./models/tags.bin
  LEX=./models/tags.lex

fi

## ================================
## ASR
## ================================

BEAM=200 ## min 150       
EVAL_MING=0.10 ## min 0.08
#
TOKEN_LIMIT=30000  
LM_SCALE=20  

if [ "$1" != "eng" ]; then ## FINNISH (default)

  ./online-demo/recognizer -b $HMM -d "./online-demo/decoder --binlm $LM --lookahead $LA_LM --lexicon $LEX --ph $HMM.ph --dur $HMM.dur --lm-scale $LM_SCALE --token-limit $TOKEN_LIMIT --beam $BEAM" -C $HMM.gcl --eval-ming $EVAL_MING

else ## ENGLISH

  if [ "$2" != "" ]; then 
    #echo "Use adapted acoustic model for user $2"
    cp ./adapt/cfgs/$2.cfg $HMM.cfg ## ADAPTED acoustic model
  else
    #echo "Use default acoustic model"
    cp ./adapt/cfgs/$HMMBASE.cfg $HMM.cfg ## DEFAULT acoustic model
  fi

  ./online-demo/recognizer -b $HMM -d "./online-demo/decoder --binlm $LM --lexicon $LEX --ph $HMM.ph --lm-scale $LM_SCALE --token-limit $TOKEN_LIMIT --beam $BEAM --words" -C $HMM.gcl --eval-ming $EVAL_MING

fi

