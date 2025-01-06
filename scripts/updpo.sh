#!/bin/sh
set -e

for po in po/*.po; do
	msgmerge "$po" "po/customfetch.pot" -o "$po"
done
