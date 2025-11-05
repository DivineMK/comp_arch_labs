#!/usr/bin/bash
rm $(dirname "$0")/bin/.*
TRANSFER=PARALLEL make && cp -f ./bin/host_code{,_parallel}
TRANSFER=SERIAL make && cp -f ./bin/host_code{,_serial}
TRANSFER=broadcast make && cp -f ./bin/host_code{,_broadcast}
