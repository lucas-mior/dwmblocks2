#!/bin/sh

ip addr | awk '
/inet .+brd/ {
    ip = gensub("/[0-9]{2}", "", "g", $2);
    print ip
}'
