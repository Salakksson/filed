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

RUN=false
INSTALL=false

for arg in echo $@;
do
	if [ "$arg" == "fresh" ] ;
	then
		rm -fr $BIN_DIR/*
	fi
	if [ "$arg" == "run" ] ;
	then
		RUN=true
	fi
	if [ "$arg" == "install" ] ;
	then
		INSTALL=true
	fi
done

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

if $INSTALL;
then
	echo sudo cp $TARGET /usr/local/bin
	sudo cp $TARGET /usr/local/bin
fi

if $RUN;
then
	$TARGET 2> error.txt
fi
