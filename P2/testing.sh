#!/bin/bash

# This assumes the tests are in another folder called "Tests"
# Compile. Comment out if you don't need this. We should be using make but let's be lazy here yeah?
gcc *.c -o gcsubc

./gcsubc Tests/test01.subc
./gcsubc Tests/test02.subc
./gcsubc Tests/test03.subc
./gcsubc Tests/test04.subc
./gcsubc Tests/test05.subc
./gcsubc Tests/test06.subc
./gcsubc Tests/test07.subc
./gcsubc Tests/test08.subc
./gcsubc Tests/test09.subc
./gcsubc Tests/test10.subc
./gcsubc Tests/test11.subc
./gcsubc Tests/test12.subc
./gcsubc Tests/test13.subc
./gcsubc Tests/test14.subc
./gcsubc Tests/test15.subc
./gcsubc Tests/test99.subc