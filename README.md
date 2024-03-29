# Multithreaded Server

> :warning: **This repository is a modified project** and thus no students of CSE 130 from UCSC should be looking at my source code. 

## Program Description

Use the `-l` to specify where to store the logs in a file. The `-t` will specify the number of threads the program will use. This program will write to a file or extract the contents of a file with the commands GET, PUT, and APPEND. APPEND is not an official request but an added request that adds contents to a file without wiping the file. The program was tested locally. Use a flag specifier to view the verbose options when sending a request or response from the server. This program uses the PTHREAD API to create separate threads for the required thread pool. This program has built-in features that deal with coherency and atomicity by the use of flock and multiple mutex locks. 

## Build

Build the program with:

 - `make`

 - `make all`

Build a single program with:

 - `make httpserver` 

## Running

To run the program:

`./httpserver` `-t thread` `-l logfile` `port number` 

## Design

For this program, the design process uses flock to deal with atomicity and coherence. Each flock call is attached in a mutex lock to keep it not being accessed in other threads and unlaced in the same subsequent way. This is done to keep it safe and make sure that other threads don't get the key to the same URI if it was called in another call. Each flock is also set up with non-blocking. In the case that another thread is holding the URI, the program will return and deal with the process at another time. This is done by checking the return value of the flock and the errno value. This program also improves how joining threads are done. Before joining threads, the program will unlock itself from other threads and make sure all threads are alert and ready to be returned to their original state. To test for race conditions and other debugging errors use the command below. 
 
`valgrind --tool=helgrind ./httpserver` `port number`
