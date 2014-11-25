make
make testbackup
rm -rf file*
./testbackup 1> output.out 2> out.out
python performance_measure.py