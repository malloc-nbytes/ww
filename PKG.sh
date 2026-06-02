#!/bin/bash

set -xe

pushd src
make clean distclean
popd

VERSION=$(cat VERSION)
FILES=$(find . -path ./.git -prune -o -type f -print)
PKGDIR="ww-${VERSION}"

mkdir -p "$PKGDIR"
cp --parents -r $FILES "$PKGDIR"

tar czf "${PKGDIR}.tar.gz" "$PKGDIR"
rm -rf "$PKGDIR"
