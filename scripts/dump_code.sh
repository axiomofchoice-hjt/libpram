#!/bin/sh

find src examples xmake.lua -type f \
  -exec echo "===== FILE: {} =====" \; \
  -exec cat {} \; \
  > build/temp

code --wait build/temp
