#!/bin/sh
# "Do not try and bend the fork. That's impossible. Instead only try to realize the truth."
# "What truth?"
# "There is no fork."

set -eu

# Hack to deal with splitting the input with read
IFS=',-
'

p1=0
p2=0

while read -r a0 a1 b0 b1 || [ -n "$b1" ]; do
	if { [ $a0 -ge $b0 ] && [ $a1 -le $b1 ]; } ||
	   { [ $b0 -ge $a0 ] && [ $b1 -le $a1 ]; }; then
		p1=$((p1 + 1))
	fi

	if [ $a0 -le $b1 ] && [ $b0 -le $a1 ]; then
		p2=$((p2 + 1))
	fi
done

echo $p1
echo $p2
