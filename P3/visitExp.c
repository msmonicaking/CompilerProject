
#include "ast.h"
#include <iostream>

void visitNum(AstNum* ast) {
   printf("%i", ast->val);
}

void visitNam(AstNam* ast) {
   printf("%s", ast->lex);
}

char* astBOPtoStr(BOP bop) {
   switch (bop) {
   case BOPADD:   return "+";
   case BOPBAD:   return "!";
   case BOPDIV:   return "/";
   case BOPEEQ:   return "==";
   case BOPGE:    return ">=";
   case BOPGT:    return ">";
   case BOPLE:    return "<=";
   case BOPLT:    return "<";
   case BOPMUL:   return "*";
   case BOPNE:    return "!=";
   case BOPNONE:  return "";
   case BOPSUB:   return "-";
   default:       return "BOPBAD";
   }
}

void visitExp(AstExp* ast) {
   
}

int main() {
   AstNum* num = astNewNum(9);
   visitNum(num);

   AstNam* nam = astNewNam("shardbearer");
   visitNam(nam);

   return 0;
}
