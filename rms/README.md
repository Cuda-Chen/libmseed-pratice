# RMS Practice
Calculate RMS (standard deviation) on certain record(s) with matched criteria.

# How to Compile and Run
```
$ make
$ ./calculate_rms.out <mseedfile> <selectionfile>
```

# Format of selectionfile
As specified in `ms3_readselectionsfile()` in [here](https://iris-edu.github.io/libmseed/group__data-selections.html#gaf8aa9de2cbbc18bc96fe3c4f0dfcb92d), you should give one of the following format:
1. `SourceID [Starttime [Endtime [Pubversion]]]`
2. `Network Station Location Channel [Pubversion [Starttime [Endtime]]]`

What more, the `Starttime` and `Endtime` values must be one of the 
format recognized by [ms_timestr2nstime()](https://iris-edu.github.io/libmseed/group__time-related.html#ga5970fa0256338e4964e25941eac6c01e)
and include a full data (i.e., just a year is not allowed).

As a special case, selection lines will be read from `stdin` if the name selectionfile is `-`.
