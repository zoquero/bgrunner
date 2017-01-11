# Summary

Lightweight tool written in C to launch and keep track of background processes

Angel Galindo Muñoz ( zoquero@gmail.com ), January of 2017

doesn't work:
[[https://github.com/zoquero/bgrunner/blob/master/doc/bgrunner_illustration.png|alt=Illustration]]

test1:
[![Illustration](https://github.com/zoquero/bgrunner/blob/master/doc/bgrunner_illustration.png)]

test2:
![Illustration](https://github.com/zoquero/bgrunner/blob/master/doc/bgrunner_illustration.png)

# Features

It allows to launch multiple processes in background, keep track of them, wait for it's completion applying timeouts and get their stdout, stderr, return code and duration.

# Motivation

Basic \*NIX tools like ampersand (`&`), `cron` and `at` just allow you to launch processes, but you have to manage them manually (`ps` and `kill` on a loop).

# Usage

`bgrunner (-v) (-d) (-o <outputfolder>) -f <jobsdescriptor>`

* `-v` == (optional) verbose
* `-d` == (optional) debug (more verbosity)
* `-o` => (optional) output folder with stdout, stderr, duration and job result code for each job. Defaults to /tmp
* `-f` => job descriptor, a CSV file like this:

`#alias;startAfterMS;maxDurationMS;command`

`one;0;0;/tmp/myscript.sh arg1 arg2`

`two;15000;10000;/usr/local/bin/mycommand myarg`

`three;5000;30000;/usr/local/bin/othercommand otherarg`

fields for the descriptor:
* 1st: alias for the job. It will be used to refer to this job on logs
* 2nd: time to wait before executing the job in miliseconds
* 3rd: max duration for the job in miliseconds. After that it will be killed sending `SIGKILL`. The launched jobs are sampled with a wait time specified on build-time (`SLEEP_TIME_US` on `bgrunner.h`), by default it's 1 milisecond.
* 4th: command to be executed with its arguments. White spaces aren't allowed on executables or args, just are allowed to split the executable and the arguments. As a workaround you can wrap it on a script and set the script as the command for the job.

# Output

It generates:

* output messages depending on the chosen verbosity (`-d` and `-v`)
* a file for stdout (`bgrunner.$job_alias.stdout`) and other for stderr (`bgrunner.$job_alias.stderr`) for each job
* a CSV file `bgrunner.results.csv` with the results of the executions

The CSV results file contains these fields for each job:

* job alias (got from the job descriptor)
* job command (got from the job descriptor)
* return code got from waitpid after the process execution (it hasn't sense if it was killed by timeout or excecve hasn't worked)
* if the process was killed by the timeout specified in the descriptor (0==false, 1==true) (it hasn't sense if it was killed by timeout)
* if execve worked (1==ok, 2==error). Typical errors: missing execution permission.
* process duration in miliseconds. Remember that it's polled periodically with a wait time specified on build-time (`SLEEP_TIME_US` on `bgrunner.h`) that by default is 1 milisecond.

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
