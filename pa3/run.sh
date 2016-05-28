#!/bin/bash
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/roman/code/fork-in-the-eye/pa3";
export LD_PRELOAD="libruntime.so";

./pa3 $@
