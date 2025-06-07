#!/bin/sh

set -e

CC=clang
CCFLAGS=" -std=c11 -D_GNU_SOURCE -Wall -Wpedantic -Werror -fsanitize=address,undefined -O0 -g"
LDFLAGS=" -lcurses -fsanitize=address,undefined"

BUILD_DIR=".build"
BIN_DIR="$BUILD_DIR/bin"
DEPFILE="$BUILD_DIR/.deps"

mkdir -p $BUILD_DIR
mkdir -p $BIN_DIR
touch $DEPFILE

if [ "$1" == "fresh" ] ;
then
	rm -fr $BIN_DIR/*
fi

TARGET=./filed

objects=""
for file in src/*.c
do
	out=${file/src/$BIN_DIR}.o
	objects="$objects $out"

	skip=true

	for dep in $(.build/fastdep.sh $file -o $out -d $DEPFILE)
	do
		if [ $file -nt $out ];
		then
			skip=false;
		fi
	done

	if $skip;
	then
		echo × skipping $file → $out
		continue
	fi

	echo $CC -c $CCFLAGS $file -o $out
	$CC -c $CCFLAGS $file -o $out
done

$CC $LDFLAGS $objects -o $TARGET
