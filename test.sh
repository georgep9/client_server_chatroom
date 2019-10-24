#!/bin/bash

temp="SUB 1\n"

for i in {1..1000}
do
	temp="$temp SEND 1 hello\n"
	
done

temp="$temp CHANNELS\n"

printf "$temp" | ./client 127.0.0.1 12345

