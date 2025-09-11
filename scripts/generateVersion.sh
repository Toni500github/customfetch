#!/bin/sh

# Shit stolen from Hyprland, ts script so gud

# if the git directory doesn't exist, don't gather data to avoid overwriting, unless
# the version file is missing altogether (otherwise compiling will fail)
if [ ! -d ./.git ]; then
    if [ -f ./include/version.h ]; then
        exit 0
    fi
fi

cp -fr ./include/version.h.in ./include/version.h

HASH=${HASH-$(git rev-parse HEAD)}
BRANCH=${BRANCH-$(git branch --show-current)}
MESSAGE=${MESSAGE-$(git log -1 --pretty=%B | head -n 1 | sed -e 's/#//g' -e 's/\"//g')}
DATE=${DATE-$(git show --no-patch --format=%cd --date=local)}
DIRTY=${DIRTY-$(git diff-index --quiet HEAD && echo clean || echo dirty)}
TAG=${TAG-$(git describe --tags)}
COMMITS=${COMMITS-$(git rev-list --count HEAD)}

sed -i -e "s#@HASH@#${HASH}#" ./include/version.h
sed -i -e "s#@BRANCH@#${BRANCH}#" ./include/version.h
sed -i -e "s#@MESSAGE@#${MESSAGE}#" ./include/version.h
sed -i -e "s#@DATE@#${DATE}#" ./include/version.h
sed -i -e "s#@DIRTY@#${DIRTY}#" ./include/version.h
sed -i -e "s#@TAG@#${TAG}#" ./include/version.h
sed -i -e "s#@COMMITS@#${COMMITS}#" ./include/version.h
