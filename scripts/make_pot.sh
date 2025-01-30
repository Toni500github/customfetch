#!/bin/sh
# original https://github.com/Morganamilo/paru/blob/master/scripts/mkpot
set -e

xgettext \
	-d customfetch \
	--msgid-bugs-address https://github.com/Toni500github/customfetch \
	--package-name=customfetch\
	--default-domain=customfetch\
	--package-version="$(awk -F '= ' '/^VERSION/ {print $2}' Makefile | sed 's/\"//g')" \
	-k_ \
	-o po/customfetch.pot \
	src/*.cpp src/query/android/*.cpp src/query/linux/*.cpp src/query/linux/utils/*.cpp --c++
