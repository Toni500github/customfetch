#!/usr/bin/env bash
set -e

if [ -z "$1" ]; then
	printf "usage: $0 (lang)\nexample: $0 en_US"
	exit 1
fi

if [ ! -f "po/customfetch.pot" ]; then
        printf "failed to get po/customfetch.pot\nplease run scripts/make_pot.sh first if haven't yet"
        exit 1
fi

if [ -f "po/$1.po" ]; then
	printf "po/$1.po already exists\nmaybe modify it if you want to contribute"
	exit 1
fi

msginit --no-translator --locale "$1" -o "po/$1.po" --input "po/customfetch.pot"
