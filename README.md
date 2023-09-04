This program uses Ush and DWM to enhance GNU R experience
on Linux/*nix.
My fork of DWM (https://github.com/BartekLew/dwm) allows to
capture keyboard events on plot window (if it's not running,
program will only display plot, keystrokes won't cause any
action). Ush (https://github.com/BartekLew/ush) allows
to multiplex input into GNU R session, so that we can
input R code through pipe as well as from terminal.

Getting started
===============

To compile project, run:

```
mkdir build
cmake -Bbuild
make -C build
```

Dependencies
============

Except Ush and DWM, GNU R and libfmt is needed.

Running
=======

Program is meant to plot financial instrument quotes. So we need
some CVS with quotes (some examples attached in `test-csv` directory).

In command line, as arguments we pass filenames of CSVs to plot,
like:

```
$ build/rgraph test-csv/*
```

At this point, helper.R file must be in running directory (as detecting
its location is not implemented yet).

There are following keystrokes supported so far:

* `+`/`-` – zoom in and zoom out plot.
* `[`/`]` – change resolution (how long period of time is represented by one "candle").
* `←`/`→` — move plot in time.
* `↑`/`↓` – switch between plots.
