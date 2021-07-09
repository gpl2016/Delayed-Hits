#!/bin/bash
ca=50
z=5
while (($z<=50))
do
	
	#build/bin/cache_lru_aggdelay --trace "data/trace.csv" --cscale $ca --zfactor $z

	build/bin/cache_belady --trace "data/trace.csv" --cscale 5 --zfactor $z > belay.txt

	z=`expr $z + 5`

done
