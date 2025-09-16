{ time bin/cache_sim traces/bzip2.log_l1misstrace 2 ; } &> logs/bzip2.txt  &
{ time bin/cache_sim traces/gcc.log_l1misstrace 2 ; } &> logs/gcc.txt  &
{ time bin/cache_sim traces/gromacs.log_l1misstrace 1 ; } &> logs/gromacs.txt &
{ time bin/cache_sim traces/h264ref.log_l1misstrace 1 ; } &> logs/h264ref.txt &
{ time bin/cache_sim traces/hmmer.log_l1misstrace 1 ; } &> logs/hmmer.txt &
{ time bin/cache_sim traces/sphinx3.log_l1misstrace 2 ; } &> logs/sphinx3.txt &