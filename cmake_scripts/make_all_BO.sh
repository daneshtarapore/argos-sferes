#!/bin/bash


for dim in 2 10 14 21 400; do
    cd ~/argos-sferes
    mkdir build_${dim}D
    cd build_${dim}D
    if [ $dim -gt 3 ]
    then
    cmake -DBO=ON -DRECORD_FITNESS=ON -DCVT_USAGE=ON -DBD=${dim} -DDEFINE_PRINT=OFF ..
    else
    cmake -DBO=ON -DRECORD_FITNESS=ON -DBD=${dim}  -DDEFINE_PRINT=OFF ..
    fi
   make -j 8


done 
