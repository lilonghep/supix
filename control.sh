#!/bin/bash
# $Id: control.sh 1088 2019-04-19 10:27:10Z mwang $
########################################################################

nohup ./control.exe "$@" > control.log 2>&1 &
tail -f control.log

