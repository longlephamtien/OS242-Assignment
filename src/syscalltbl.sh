#!/bin/sh

# Copyright (C) 2025 pdnguyen of HCMC University of Technology
# Sierra release
# Source Code License Grant: The authors hereby grant to Licensee 
# personal permission to use and modify the Licensed Source Code 
# for the sole purpose of studying while attending the course CO2018.

set -e

usage() {
	echo >&2 "usage: $0 INFILE OUTFILE" >&2
	echo >&2
	echo >&2 "  INFILE    input syscall table"
	echo >&2 "  OUTFILE   output lst file"
	echo >&2
	exit 1
}


if [ $# -ne 2 ]; then
	usage
fi

infile="$1"
outfile="$2"

nxt=0

grep -E "^[0-9]+[[:space:]]+" "$infile" | sort -n | {

	while read nr name native ; do

		if [ -n "$native" ]; then
			echo "__SYSCALL($nr, $native)"
		fi
		nxt=$((nr + 1))
	done
} > "$outfile"
