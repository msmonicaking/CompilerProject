// emit.h - Emit 68000 Assembly Code - Jim Hogg, 2020

#pragma once

#include <assert.h>     // assert
#include <stdio.h>      // sprintf

#include "ut.h"         // ut*

typedef struct {
  #define CODESIZE 5000
  char* codeBuf;
  int   codeSize;

  #define DATASIZE 5000
  char* dataBuf;
  int   dataSize;
} Emit;

void  emitCode(Emit* emit, char* line);
void  emitData(Emit* emit, char* line);
void  emitDump(Emit* emit);
Emit* emitNew();
char* emitNewName(char* sourcePath);
void  emitSave(Emit* emit, char* filePath);