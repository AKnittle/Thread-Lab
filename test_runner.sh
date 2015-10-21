#!/bin/bash
# runs a bunch of small tests with numerous testing scenarious, for the thread pool.
echo Basic test...
echo 5 threads
./threadpool_test -n 5
./threadpool_test -n 5
./threadpool_test -n 5
./threadpool_test -n 5
./threadpool_test -n 5

echo 4 threads
./threadpool_test -n 4
./threadpool_test -n 4
./threadpool_test -n 4
./threadpool_test -n 4
./threadpool_test -n 4

echo 3 threads
./threadpool_test -n 3
./threadpool_test -n 3
./threadpool_test -n 3
./threadpool_test -n 3
./threadpool_test -n 3

echo 2 threads
./threadpool_test -n 2
./threadpool_test -n 2
./threadpool_test -n 2
./threadpool_test -n 2
./threadpool_test -n 2

echo 1 thread
./threadpool_test
./threadpool_test
./threadpool_test
./threadpool_test
./threadpool_test

echo Basic test 2...
echo 5 threads
./threadpool_test2 -n 5
./threadpool_test2 -n 5
./threadpool_test2 -n 5
./threadpool_test2 -n 5
./threadpool_test2 -n 5

echo 4 threads
./threadpool_test2 -n 4
./threadpool_test2 -n 4
./threadpool_test2 -n 4
./threadpool_test2 -n 4
./threadpool_test2 -n 4

echo 3 threads
./threadpool_test2 -n 3
./threadpool_test2 -n 3
./threadpool_test2 -n 3
./threadpool_test2 -n 3
./threadpool_test2 -n 3

echo 2 threads
./threadpool_test2 -n 2
./threadpool_test2 -n 2
./threadpool_test2 -n 2
./threadpool_test2 -n 2
./threadpool_test2 -n 2

echo 1 thread
./threadpool_test2 
./threadpool_test2 
./threadpool_test2 
./threadpool_test2 
./threadpool_test2

echo Basic test 3...
echo 5 threads
./threadpool_test3 -n 5
./threadpool_test3 -n 5
./threadpool_test3 -n 5
./threadpool_test3 -n 5
./threadpool_test3 -n 5

echo 4 threads
./threadpool_test3 -n 4
./threadpool_test3 -n 4
./threadpool_test3 -n 4
./threadpool_test3 -n 4
./threadpool_test3 -n 4

echo 3 threads
./threadpool_test3 -n 3
./threadpool_test3 -n 3
./threadpool_test3 -n 3
./threadpool_test3 -n 3
./threadpool_test3 -n 3

echo 2 threads
./threadpool_test3 -n 2
./threadpool_test3 -n 2
./threadpool_test3 -n 2
./threadpool_test3 -n 2
./threadpool_test3 -n 2

echo 1 thread
./threadpool_test3 
./threadpool_test3 
./threadpool_test3 
./threadpool_test3 
./threadpool_test3 

 



