#!/bin/sh
# run the test vs test4x4.png
# galena 
#   infile.png
#   population_count, max_generations, num_genes, mutation_rate, fitness_goal \
#   random_seed
POPC=64
MAXG=100
NGEN=100
MUTR=0.25
FITG=10000.0
RAND=`date "+%H%M%S"`
AUTQ=1
DIR=out/test_lena_${POPC}_${MAXG}_${NGEN}_${MUTR}_${FITG}_${RAND}
mkdir -p $DIR
../bin/release/galena \
  data/lena.png $POPC $MAXG $NGEN $MUTR $FITG $RAND $AUTQ | \
  tee ${DIR}/test.log
mv galena_out.png ${DIR}/test.png

