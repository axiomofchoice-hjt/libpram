#!/bin/sh

find src examples xmake.lua README.md -type f \
  -exec echo "===== FILE: {} =====" \; \
  -exec cat {} \; \
  > build/temp

code --wait build/temp
