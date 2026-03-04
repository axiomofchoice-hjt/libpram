#!/bin/sh

find . -type f \
  ! -path '*/.git/*' \
  ! -path '*/build/*' \
  ! -path '*/.cache/*' \
  ! -path '*/.xmake/*' \
  ! -path '*/.github/*' \
  ! -path '*/.clang-*' \
  -exec echo "===== FILE: {} =====" \; \
  -exec cat {} \; \
  > build/temp

code build/temp
