#!/bin/bash
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/bildeyko/dev/reps/distcomp/pa2/lib32";
export LD_PRELOAD="libruntime.so";

./pa2 $@
