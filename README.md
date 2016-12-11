# Summary

Tool written in C to launch background processes

Angel Galindo ( zoquero@gmail.com ), december of 2016

# Features

It allows you to launch multiple processes in background, keep track of them and wait for it's completion. You can set an offset of time before starting each job and you can appply a timeout for each.

# Motivation

Basic \*NIX tools are too much basic:

* With ampersand (`&`) you can just launch one process and you have manage it by hand (`ps` and `kill` on a loop).
* `cron` and `at` allow you to run multiple background processes but you still have to manage them manually.

# Build and install

## Quick guide

How to build:
* (`make clean`)
* `make`

How to install:
* `sudo install -o root -g root -m 0755 ./bgrunner /usr/bin/`

## Dependencies

None

# Source
It's source can be found at GitHub: [https://github.com/zoquero/bgrunner/](https://github.com/zoquero/bgrunner/).

I hope you'll find it useful!

/Ángel
