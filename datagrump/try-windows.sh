#!/bin/bash -eux

function run()
{
    touch controller.cc
    make CPPFLAGS=-DWINDOW_SIZE=$1 && ./run-contest fixed-window
}

run 1
run 1
run 2
run 2
run 5
run 5
run 10
run 10
run 20
run 20
run 40
run 40
run 50
run 50
run 80
run 80
run 100
run 100
run 200
run 200
run 500
run 500
run 1000
run 1000
run 2000
