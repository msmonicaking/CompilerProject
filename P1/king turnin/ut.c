// ut.c - Utility functions for the SubC Compiler - Jim Hogg, 2020 & modified by Monica King 2022

#define _CRT_SECURE_NO_DEPRECATE

#include "ut.h"

void utDie2Str(char* func, char* msg) {
  printf("\n\nERROR: %s: %s \n\n", func, msg);
  utPause();
}

void utDie3Str(char* func, char* msg1, char*msg2) {
  printf("\n\nERROR: %s: %s %s \n\n", func, msg1, msg2);
  utPause();
}

void utDie4Str(char* func, char* msg1, char* msg2, char* msg3) {
  printf("\n\nERROR: %s: %s %s %s \n\n", func, msg1, msg2, msg3);
  utPause();
}

void utDie2StrCharLC(char* func, char* msg, char c, int linNum, int colNum) {
  printf("\n\nERROR: %s %s %c at (%d, %d) \n\n",
    func, msg, c, linNum, colNum);
  utPause();
}

void utDieStrTokStr(char* func, Tok* tok, char* msg) {
  printf("\n\nERROR: %s: Found %s but expecting %s at (%d, %d) \n\n",
    func, tokStr(tok->kind), msg, tok->linNum, tok->colNum);
  utPause();
}

void utPause() {
  printf("Hit any key to finish");
  getchar();
  exit(0);
}

// Read the entire file specified by 'filePath'.  Return its contents.
char* utReadFile(char* filePath) {

   FILE* fp = fopen(filePath, "r");

   if (!fp) {
      printf("Unable to open file");
      exit(0);
   }

   fseek(fp, 0L, SEEK_END);
   int sz = ftell(fp);
   fseek(fp, 0L, SEEK_SET);

   char* prog = (char*) calloc(sz, 1);
   fread(prog, 1, sz, fp);

   fclose(fp);

   return prog;
}

char* utStrndup(char* s, int len) {
  char* copy = malloc(len + 1);
  strncpy(copy, s, len);
  copy[len] = '\0';
  return copy;
}
