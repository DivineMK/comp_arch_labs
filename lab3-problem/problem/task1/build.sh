#!/usr/bin/bash
rm $(dirname "$0")/bin/.*
for TRANSFER in SERIAL PARALLEL BROADCAST; do
  transfer=${TRANSFER,,}
  for i in 1 2 4 8 16 32 64; do
    TRANSFER=$TRANSFER NR_DPUS=$i make && cp -f ./bin/host_code{,_${transfer}_${i}}
  done
done
