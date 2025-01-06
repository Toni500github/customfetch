#!/bin/bash
# original https://github.com/Morganamilo/paru/blob/master/scripts/mkmo
set -e

if [ -z "$1" ]; then
	echo "usage: $0 <dir>"
	exit 1
fi

for po in po/*.po; do
	lang=$(basename ${po%.po})
	install -dm755 "$1/$lang/LC_MESSAGES/"
	msgfmt "$po" -o "$1/$lang/LC_MESSAGES/customfetch.mo"
done
