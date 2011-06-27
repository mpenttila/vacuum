#!/bin/sh

mkdir -p logs
./distort --css style.css --automaticrotation --players 1 2>&1 | tee logs/`date +%Y-%m-%d-%H%M%S`_automatic_tight.log
