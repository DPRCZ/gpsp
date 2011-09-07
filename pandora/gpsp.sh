#!/bin/sh

if [ ! -f romdir.txt ]; then
  echo -n "/media/" > romdir.txt
fi
sudo -n /usr/pandora/scripts/op_lcdrate.sh 60

./gpsp "$@"

# restore stuff in case of crash
./picorestore
