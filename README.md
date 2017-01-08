# Summary

Tool written in C to launch and keep track of background processes

Angel Galindo ( zoquero@gmail.com ), January of 2017

# Features

It allows to launch multiple processes in background, keep track of them, wait for it's completion applying timeouts and get their stdout, stderr, return code and duration.

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
* `gzip ./doc/bgrunner.1.gz`
* `sudo install -o root -g root -m 0644 ./doc/bgrunner.1.gz /usr/share/man/man1/`


## Dependencies

None

# Source
It's source can be found at GitHub: [https://github.com/zoquero/bgrunner/](https://github.com/zoquero/bgrunner/).

I hope you'll find it useful!

/Ángel
