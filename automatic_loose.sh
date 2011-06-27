#!/bin/sh

mkdir -p logs
./vacuum --css style.css --automaticrotation --players 2 2>&1 | tee logs/`date +%Y-%m-%d-%H%M%S`_automatic_loose.log
