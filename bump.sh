#!/bin/sh

# Use passed argument as new tag
if [ $# -eq 0 ]; then
  version=$(git describe --tags --abbrev=0)
  patch=${version##*.}
  set -- "${version%.*}.$((patch+1))"
fi

git tag "$@" || exit 1

tag_curr="$(git tag --sort=version:refname | tail -1)"
tag_prev="$(git tag --sort=version:refname | tail -2 | head -1)"

./version.sh "$tag_curr"

sed -r "s/${tag_prev}/${tag_curr}/g" -i \
  README.md CMakeLists.txt \
  contrib/polybar.aur/PKGBUILD contrib/polybar.aur/.SRCINFO \
  contrib/polybar-git.aur/PKGBUILD contrib/polybar-git.aur/.SRCINFO

git add -u README.md CMakeLists.txt \
  contrib/polybar.aur/PKGBUILD contrib/polybar.aur/.SRCINFO \
  contrib/polybar-git.aur/PKGBUILD contrib/polybar-git.aur/.SRCINFO \
  include/version.hpp

git commit -m "build: Bump version to ${tag_curr}"

# Recreate the tag to include the last commit
[ $# -eq 1 ] && git tag -f "$@"
