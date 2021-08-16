#!/bin/sh

gcc -static debug.c init.c -o init -lrt -lpthread
