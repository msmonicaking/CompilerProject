// main.h - SubC Compiler - Jim Hogg, 2020 & modified by Monica King 2022

#pragma once

#include <stdio.h>      // printf, FILE
#include <stdlib.h>     // exit

#include "lex.h"        // Lex
#include "ut.h"         // utReadFile

int main(int argc, char* argv[]);
void usage();
