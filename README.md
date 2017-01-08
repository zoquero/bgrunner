# Summary

Lightweight tool written in C to launch and keep track of background processes

Angel Galindo Muñoz ( zoquero@gmail.com ), January of 2017

# Features

It allows to launch multiple processes in background, keep track of them, wait for it's completion applying timeouts and get their stdout, stderr, return code and duration.

# Motivation

Basic \*NIX tools are too much basic:

* With ampersand (`&`) you can just launch one process and you have manage it by hand (`ps` and `kill` on a loop).
* `cron` and `at` allow you to run multiple background processes but you still have to manage them manually.

# Usage

`bgrunner (-v) (-d) (-o <outputfolder>) -f <jobsdescriptor>`

* `-v` == verbose (optional)
* `-d` == debug (optional, more verbosity)
* `-o` => output folder with stdout, stderr, duration and job result code for each job. Defaults to /tmp
* `-f` => job descriptor, a CSV file like this:

`#alias;startAfterMS;maxDurationMS;command`

`one;0;0;/tmp/myscript.sh`

`two;15000;10000;/usr/local/bin/myexecutable`

`three;5000;30000;/usr/local/bin/otherexecutable`

fields for the descriptor:
* 1st: alias for the job
* 2nd: miliseconds to wait before executing the job
* 3rd: max duration for the job in miliseconds. After that it will be killed sending `SIGKILL`.
* 4th: path for the command to be executed. Parameters aren't allowed. As a workaround you can wrap them on a script.

# Output

It generates:
* output messages depending on the chosen verbosity (`-d` and `-v`)
* a file for stdout (`bgrunner.$job_alias.stdout`) and other for stderr (`bgrunner.$job_alias.stderr`) for each job
* a CSV file `bgrunner.results.csv` with the results of the executions

The CSV results file contains these fields for each job:
* job alias (got from the job descriptor)
* job command (got from the job descriptor)
* return code got from waitpid after the process execution
* if the process was killed by the timeout specified in the descriptor (0==false, 1==true)
* if execl worked (1==ok, 2==error). Typical errors: missing execution permission.
* process duration in miliseconds. Keep in mind that it's polled each 10 miliseconds, you can modify this sample interval changing `US_TO_SHOW_ON_DEBUG` on `bgrunner.h` and rebuilding.

# Build and install

## Quick guide

How to build:
* (`make clean`)
* `make`

How to install:
* `sudo install -o root -g root -m 0755 ./bgrunner /usr/bin/`
* `gzip ./doc/bgrunner.1`
* `sudo install -o root -g root -m 0644 ./doc/bgrunner.1.gz /usr/share/man/man1/`

## Open Build Service
If you prefer you can install my already built packages available in [Open Build Service](https://build.opensuse.org/package/show/home:zoquero:bgrunner/bgrunner).

## Dependencies

None

# Source
It's source can be found at GitHub: [https://github.com/zoquero/bgrunner/](https://github.com/zoquero/bgrunner/).

I hope you'll find it useful!

/Ángel
