#!/bin/sh

mkdir -p logs
./distort --css style.css 2&>1 | tee logs/`date +%Y-%m-%d-%H%M%S`_automatic_loosely.log

