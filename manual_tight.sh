#!/bin/sh

mkdir -p logs
./distort --css style.css --manualrotation --players 1 2>&1 | tee logs/`date +%Y-%m-%d-%H%M%S`_manual_tight.log

