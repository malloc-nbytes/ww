#!/bin/bash

set -xe

cc -o ww-debug-build -O0 -ggdb *.c -Iinclude/
