#!/bin/bash

BENCHMARKS="perlbench bzip2 gcc bwaves mcf milc zeusmp gromacs cactusADM leslie3d namd gobmk dealII soplex povray calculix hmmer sjeng GemsFDTD libquantum h264ref tonto lbm omnetpp wrf sphinx3"
haha(){
	for benchmark in $BENCHMARKS
	do
		./build/X86/gem5.opt configs/example/se_try.py -I 50000000 --caches --l1d_size=32kB --l1d_assoc=2 --l1i_size=32kB --l1i_assoc=2 --l2cache --l2_size=256kB --l2_assoc=8 --cacheline_size=64 --cpu-type=DerivO3CPU --mem-type=SimpleMemory --mem-size=8192MB -n 1 --maxinsts=10000000 --bench=${benchmark}
		mv ./m5out/stats.txt ./m5out/${benchmark}_no.txt
		./build/X86/gem5.opt configs/example/se_try.py -I 50000000 --caches --l1d_size=32kB --l1d_assoc=2 --l1i_size=32kB --l1i_assoc=2 --l2cache --l2_size=256kB --l2_assoc=8 --cacheline_size=64 --cpu-type=DerivO3CPU --mem-type=SimpleMemory --mem-size=8192MB -n 1 --maxinsts=10000000 --bench=${benchmark} --prefetchers stride
                mv ./m5out/stats.txt ./m5out/${benchmark}_stride.txt
		./build/X86/gem5.opt configs/example/se_try.py -I 50000000 --caches --l1d_size=32kB --l1d_assoc=2 --l1i_size=32kB --l1i_assoc=2 --l2cache --l2_size=256kB --l2_assoc=8 --cacheline_size=64 --cpu-type=DerivO3CPU --mem-type=SimpleMemory --mem-size=8192MB -n 1 --maxinsts=10000000 --bench=${benchmark} --prefetchers tagged
		mv ./m5out/stats.txt ./m5out/${benchmark}_tagged.txt
	done
}

haha



