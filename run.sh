#!/bin/bash
ca=5
z=5
while (($z<=50000))
do
	
	build/bin/cache_lru_aggdelay --trace "data/trace.csv" --cscale $ca --zfactor $z > 0629shell
	z=`expr $z + 5`

done
