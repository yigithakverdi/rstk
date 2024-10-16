#!/bin/bash

KRILL="krill"
DATA_DIR="/etc/krill"
KRILL_PID="$DATA_DIR/krill.pid"
CONF="$DATA_DIR/krill.conf"
SCRIPT_OUT="$DATA_DIR/krill.log"

nohup $KRILL -c $CONF >$SCRIPT_OUT 2>&1 &
echo $! > $KRILL_PID
