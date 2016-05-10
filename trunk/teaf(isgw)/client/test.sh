#!/bin/bash

uin=29320360
while true
do
    ./client 172.0.0.1 5693 "cmd=1&uin=$uin" 1000000 1 & 
    uin=`expr $uin + 1`
    sleep 5
done 

