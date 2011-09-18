#!/bin/sh

mkdir -p logs
./vacuum --css style.css --players 2 --displaywidth 518 2>&1 | tee logs/`date +%Y-%m-%d-%H%M%S`_2player.log

