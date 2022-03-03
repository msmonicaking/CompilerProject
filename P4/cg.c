// cg.c - Code Generator for SubC ASTs - Jim Hogg, 2020

#include "cg.h"

// ============================================================================
// Asg        => Nam "=" (Exp | Call) ";"
// NamNum     => Nam | Num
// Exp        => NamNum | NamNum Bop NamNum
// Bop        => "+" | "-" | "*" | "<" | "<=" | "!=" | "==" | ">=" | ">"
//
// Suppose "i = add3(first, 5, third)".  Then cgAsg will generate the
// following 68000 code:
//
//   PUSH.L (@first,A6)
//   PUSH.L #5
//   PUSH.L (@third,A6)
//   BSR    add3
//
// In the above code, "@a" represents the offset, in bytes, of argument "a"
// from its Frame Pointer (FP = A6)
//
// Generate code to copy the value in D0 to the variable called 'varnam'.
// For example, to compile: "mx = 2" we will have moved #2 into D0.  Then
// cgAsg will copy the value in D0 into the variable called 'varnam' that is
// defined in the function called 'funnam'
// ============================================================================
void cgAsg(Cg* cg, char* funnam, char* varnam) {
  char line[LINESIZE];

  int varidx = layFindVarParIdx(cg->lay, funnam, varnam);
  assert (varidx != 0);

  int varoff = cg->lay->row[varidx].off;

  sprintf(line, "\t %s \t %s%d%s", "MOVE.L", "D0, (", varoff, ",A6)");
  emitCode(cg->emit, line);
}

// ============================================================================
// Block => "{" Stm+ "}"
// ============================================================================
void cgBlock(Cg* cg, char* funnam, AstBlock* astblock) {
  AstStm* aststm = astblock->stms;
  while(aststm) {
    cgStm(cg, funnam, aststm);
    aststm = (AstStm*)aststm->next;
  }
}

// ============================================================================
// Body => "{" Var* Stm+ "}"
// ============================================================================
void cgBody(Cg* cg, char* funnam, AstBody* astbody) {
  cgStms(cg, funnam, astbody->stms);
}

// ============================================================================
// Generate code for the operation (bop) connecting D0 and D1.  If 'bop' is an
// arithmetic operator (+ - * /) then the answer is generated into D0.
// For example, "a + b" generates:
//
//    MOVE.L  (@a,A6), D0
//    MOVE.L  (@b,A6), D1
//    ADD.L   D1, D0
//
// If 'bop' is a comparison operator (< <= == != >= >), D0 will hold 1 for
// TRUE and 0 for FALSE.  For example, "a < 5" generates:
//
//      MOVE.L  (@a,A6), D0
//      MOVE.L  #5, D1
//      CMP.L   D1, D0
//      BLT     L20
//      MOVE.L  #0, D0      ; FALSE
//      BRA     L30
// L20: MOVE.L  #1, D0      ; TRUE
// L30:
//
// ============================================================================
void cgBop(Cg* cg, BOP bop) {
  char line[LINESIZE];

  // First process Arithmetic operators

  if (bop == BOPADD) {
    sprintf(line, "\t %s \t %s", "ADD.L", "D1, D0");
    emitCode(cg->emit, line);
    return;
  } else if (bop == BOPMUL) {
    sprintf(line, "\t %s \t %s", "MULS", "D1, D0");
    emitCode(cg->emit, line);
    return;
  } else if (bop == BOPSUB) {
    sprintf(line, "\t %s \t %s", "SUB.L", "D1, D0");
    emitCode(cg->emit, line);
    return;
  }

  // Now process the Boolean operators.

  sprintf(line, "\t %s \t %s", "CMP.L", "D1, D0");
  emitCode(cg->emit, line);

  if (bop == BOPLT) {
    cgBranch(cg, "BLT");
  } else if (bop == BOPLE) {
    cgBranch(cg, "BLE");
  } else if (bop == BOPEEQ) {
    cgBranch(cg, "BEQ");
  } else if (bop == BOPNE) {
    cgBranch(cg, "BNE");
  } else if (bop == BOPGE) {
    cgBranch(cg, "BGE");
  } else if (bop == BOPGT) {
    cgBranch(cg, "BGT");
  }

}

// ============================================================================
// Generate a conditional branch.
//
// 'cond' is a conditional branch instruction such as "BNE" or "BGT".
//
// 'bop' is a comparison operator (< <= == != >= >).  Make D0 hold 1 for
// TRUE and 0 for FALSE.  For example, "BLE" generates:
//
//        BLE     L10
//        CLR.L   D0
//        BRA     L20
//  L10:  MOVE.L  #1, D0
//  L20:
// ============================================================================
void cgBranch(Cg* cg, char* cond) {
  char line[LINESIZE];

  char* truelabel = cgLabel();
  sprintf(line, "\t %s \t %s", cond, truelabel);      // eg: L10
  emitCode(cg->emit, line);

  sprintf(line, "\t %s \t %s", "CLR.L", "D0");        // FALSE
  emitCode(cg->emit, line);

  char* exitlabel = cgLabel();                        // eg: L20
  sprintf(line, "\t %s \t %s", "BRA", exitlabel);
  emitCode(cg->emit, line);

  sprintf(line, "%s:", truelabel);                    // eg: L10:
  emitCode(cg->emit, line);

  sprintf(line, "\t %s \t %s", "MOVE.L", "#1, D0");   // TRUE
  emitCode(cg->emit, line);

  sprintf(line, "%s:", exitlabel);                    // eg: L20
  emitCode(cg->emit, line);
}

// ============================================================================
// Call => Nam "(" Args ")"
//
// Emit code to push arguments (right to left), and to BSR to the target
// function.  For example:
//
//    int add2(int a, int b) { ... }
//    int main() { int mx; int my; int ms; ... ms = add2(mx, my); ... }
//
//    caller  =  "main" = caller function name
//    callee  =  "add2" = callee function name
//    argname =  "my"   = name of argument #2 in caller function 'main'
//            => "mx"   = name of argument #1 in caller function 'main'
//    numarg  =  2      = number of arguments to push (mx and my)
//    argnum  =  2      = argument number for "my"
//            => 1      = argument number for "mx"
//
// Needless to say, the number of arguments supplied by "main" in the
// call to "add2" must match the number of parameters in the definition
// of function "add2".  This version of the SubC compiler does NOT include
// any check for this.
//
// "funnam" is the name of the current function - the one that emits the BSR.
// ============================================================================
void cgCall(Cg* cg, char* funnam, AstCall* astcall) {
  char line[LINESIZE];
  Lay* lay = cg->lay;                                     // alias

  char* callee = astcall->nam->lex;                       // eg: "add2"

  int numarg = astCountArgs(astcall->args);               // eg: 2

  for (int argnum = numarg; argnum >= 1; --argnum) {
    AstArg* astarg = astFindArg(astcall->args, argnum);
    assert(astarg);

    // What kind of argument is this?  Nam, Num or Str?

    if (astarg->nns->kind == ASTNAM) {                      // var|par
      AstNam* astnam = (AstNam*) astarg->nns;
      char* lex = astnam->lex;                              // eg: "my"

      int argidx = layFindVarParIdx(lay, funnam, lex);      // eg: "main", "my"
      assert (argidx != 0);

      int argoff = lay->row[argidx].off;

      sprintf(line, "\t %s \t %s%d%s%s",
        "MOVE.L", "(", argoff, ",A6)", ", -(A7)");          // eg: MOVE.L (-12,A6),-(A7)
      emitCode(cg->emit, line);

    } else if (astarg->nns->kind == ASTNUM) {               // literal number
      AstNum* num = (AstNum*) astarg->nns;
      int val = num->val;                                   // eg: 42
      sprintf(line, "\t %s \t %s%d%s",
        "MOVE.L", "#", val, ", -(A7)");                     // eg: MOVE.L #42,-(A7)
      emitCode(cg->emit, line);
    } else if (astarg->nns->kind == ASTSTR) {               // literal string
      char* datalabel = cgLabel();
      sprintf(line, "%s:", datalabel);                      // eg: L50:
      emitData(cg->emit, line);

      AstStr* str = (AstStr*) astarg->nns;
      char*   txt = str->txt;
      sprintf(line, "\t %s \t '%s',0", "DC.B", txt);
      emitData(cg->emit, line);

      sprintf(line, "\t %s \t %s%s", "LEA", datalabel, ", A0");
      emitCode(cg->emit, line);

      sprintf(line, "\t %s \t %s%s", "MOVE.L", "A0, ", "-(A7)");
      emitCode(cg->emit, line);
    }
  }

  sprintf(line, "\t %s \t%s", "BSR", callee);             // eg: "BSR add2"
  emitCode(cg->emit, line);

  // Remember to remove the arguments previously pushed onto the stack.
  // Because each stack slot in 68000 is a Longword, the number of bytes
  // (numb) to remove is simply 4 * numarg

  int numb = 4 * numarg;

  sprintf(line, "\t %s \t#%d%s", "ADD.L", numb, ", A7");
  emitCode(cg->emit, line);

}

// ============================================================================
// Generate the Epilog for the function called 'funnam'
// ============================================================================
void cgEpilog(Cg* cg, char* funnam) {

  Emit* emit = cg->emit;                // alias

  char line[LINESIZE];

  // Remove the stack space previously reserved for local variables

  sprintf(line, "\t %s \t %s", "MOVEA.L", "A6, A7");
  emitCode(emit, line);

  // Restore the old Frame Pointer (FP = A6)

  sprintf(line, "\t %s \t %s", "MOVEA.L", "(A6), A6");
  emitCode(emit, line);

  // Point SP at return-address

  sprintf(line, "\t %s \t %s", "ADDA.L", "#4, A7");
  emitCode(emit, line);

  // Emit the RTS or SIMHALT instruction

  if (strcmp(funnam, "main") == 0) {
    sprintf(line, "\t %s", "SIMHALT");
    emitCode(emit, line);
  } else {
    sprintf(line, "\t %s", "RTS");
    emitCode(emit, line);
  }

}

// ============================================================================
// Exp => NamNum | NamNum Bop NamNum
//
// Suppose Exp = a - 7.  Then cgExp will generate the following 68000 code:
//
//   MOVE.L (@a,A6), D0
//   MOVE.L #7, D1
//   SUB.L  D1, D0
//
// 'funnam' is the name of the function in which this expression occurs.
// ============================================================================
void cgExp(Cg* cg, char* funnam, AstExp* astexp) {

  if (astexp->lhs == NULL) return;

  if (astexp->lhs->kind == ASTNAM) {
    AstNam* astnam = (AstNam*) (astexp->lhs);
    cgNam(cg, funnam, astnam, "D0");
  } else if (astexp->lhs->kind == ASTNUM) {
    AstNum* astnum = (AstNum*) (astexp->lhs);
    cgNum(cg, astnum, "D0");
  }

  if (astexp->rhs == NULL) return;

  if (astexp->rhs->kind == ASTNAM) {
    AstNam* astnam = (AstNam*) (astexp->rhs);
    cgNam(cg, funnam, astnam, "D1");
  } else if (astexp->rhs->kind == ASTNUM) {
    AstNum* astnum = (AstNum*) (astexp->rhs);
    cgNum(cg, astnum, "D1");
  }

  cgBop(cg, astexp->bop);

}

// ============================================================================
// Fun => "int"   Nam     "(" Pars ")" Body
//      | "int"   "main"  "("      ")" Body
//
// Note that we need to devise the frame Layout in order to know where to find
// each Argument and local Variable in the Stack Frame.
// ============================================================================
void cgFun(Cg* cg, AstFun* astfun) {
  layBuild(cg->lay, astfun);                // build layout (par/var offsets)
  char* funnam = astfun->nam->lex;          // name of current function

  // Emit the label that marks the start location of this function.  For
  // example, if 'funnam' = "add2" then emit the line: "add2: "

  char line[LINESIZE];
  sprintf(line, "%s:", funnam);
  emitCode(cg->emit, line);

  // Emit the Prolog code

  cgProlog(cg, funnam);

  // Now generate code for the body of the function

  cgBody(cg, funnam, astfun->body);         // generate code for body

}

// ============================================================================
// If => "if" "(" Exp ")" Block
// ============================================================================
void cgIf(Cg* cg, char* funnam, AstIf* astif) {
  char line[LINESIZE];

  // Note that cgExp returns its answer in D0 if this is an arithmetic
  // expression.  If it's a Boolean expression, then the answer is returned
  // in D0 with TRUE = 1 or FALSE = 0

  //++ Complete this function

}

// ============================================================================
// Generate a fresh label.  The sequence generated is L10, L20, L30, etc
// ============================================================================
char* cgLabel() {
  #define LABELINC 10;
  static int labnum = 10;

  char* line = calloc(LINESIZE, 1);
  assert(line);

  labnum += LABELINC;
  sprintf(line, "L%d", labnum);
  return line;
}

// ============================================================================
// Nam => Alpha AlphaNum*
//
// Suppose astnam->nam = "x" and reg = "D1".  Then lookup the offset, from FP,
// of parameter or local variable "x".  If found, emit code: "MOVE.L x, D1"
// using "MOVE.L (offset,A6), D1"
// ============================================================================
void cgNam(Cg* cg, char* funnam, AstNam* astnam, char* reg) {

  char line[LINESIZE];

  int idx = layFindVarParIdx(cg->lay, funnam, astnam->lex);

  int off = cg->lay->row[idx].off;
  if (off == 0) utDie5Str("cgNam", "cgFind failed, looking for symbol",
    astnam->lex, "in function", funnam);

  sprintf(line, "\t %s \t %s%d%s %s", "MOVE.L", "(", off, ",A6),", reg);
  emitCode(cg->emit, line);
}

// ============================================================================
// Build a new Cg (CodeGen) struct
// ============================================================================
Cg* cgNew() {
  Cg* cg = calloc(sizeof(Cg), 1);
  assert(cg);

  cg->lay = layNew(LAYMAX);
  cg->emit = emitNew();

  return cg;
}

// ============================================================================
// Num => [0-9]+
//
// Suppose astnum->num = 42, and reg = "D1".  Then emit: "MOVE.L #42, D1"
// ============================================================================
void cgNum(Cg* cg, AstNum* astnum, char* reg) {
  char line[LINESIZE];
  sprintf(line, "\t %s \t %s%d%s %s", "MOVE.L", "#", astnum->val, ",", reg);
  emitCode(cg->emit, line);
}

// ============================================================================
// Prog => Fun+
// ============================================================================
void cgProg(Cg* cg, AstProg* astprog) {
  AstFun* astfun = astprog->funs;

  // Write out "INCLUDE io.X68" to the output assembly buffer

  char line[LINESIZE];
  sprintf(line, "\t %s \t %s", "INCLUDE", "..\\..\\Tests\\io.X68");
  emitCode(cg->emit, line);

  // Pre-populate the Layout with intrinsics says, sayn and sayl

  layBuildIntrinsics(cg->lay);

  // Generate code for each function we encounter (in lexical order)
  // in the SubC source file

  while (astfun) {
    cgFun(cg, astfun);
    astfun = (AstFun*) (astfun->next);
  }

  sprintf(line, "\t %s \t %s", "END", "main");
  emitCode(cg->emit, line);

}

// ============================================================================
// Emit Prolog code for a function.  For example:
//
//    int add2(int a, int b) { ... }
//    int main() { int mx; int my; int ms; ... ms = add2(mx, my); ... }
//
// 'funnam' is the name of the current function
// ============================================================================
void cgProlog(Cg* cg, char* funnam) {

  //++ Complete this function

}

// ============================================================================
// Stm => If | Asg | Ret | While
// ============================================================================
void cgStm(Cg* cg, char* funnam, AstStm* aststm) {
  switch(aststm->kind) {
    case ASTIF:     { AstIf* astif = (AstIf*) aststm;
                      cgIf(cg, funnam, astif);
                      break;
                    }
    case ASTASG:    { AstAsg* astasg = (AstAsg*) aststm;
                      if (astasg->eoc->kind == ASTCALL) {             // Call
                        AstCall* astcall = (AstCall*) astasg->eoc;
                        cgCall(cg, funnam, astcall);
                      } else {                                        // Exp
                        AstExp* astexp = (AstExp*) astasg->eoc;
                        cgExp(cg, funnam, astexp);
                      }
                      cgAsg(cg, funnam, astasg->nam->lex);
                      break;
                    }
    case ASTRET:    { AstRet* astret = (AstRet*) aststm;
                      cgExp(cg, funnam, astret->exp);
                      cgEpilog(cg, funnam);
                      break;
                    }
    case ASTWHILE:  { AstWhile* astwhile = (AstWhile*) aststm;
                      cgWhile(cg, funnam, astwhile);
                      break;
                    }
    default:        { utDie2Str("cgStm", "Invalid aststm->kind"); }
  }
}

// ============================================================================
// Stms = Stm+
// ============================================================================
void cgStms(Cg* cg, char* funnam, AstStm* aststm) {
  while (aststm) {
    cgStm(cg, funnam, aststm);
    aststm = (AstStm*) aststm->next;
  }
}

// ============================================================================
// While => "while" "(" Exp ")" Block
// ============================================================================
void cgWhile (Cg* cg, char* funnam, AstWhile* astwhile) {
  char line[LINESIZE];

  char* startlabel = cgLabel();                     // eg: "L20"
  sprintf(line, "%s%s", startlabel, ":");           // start label
  emitCode(cg->emit, line);

  char* exitlabel = cgLabel();                      // eg: "L30"

  cgExp(cg, funnam, astwhile->exp);                 // result in D0

  sprintf(line, "\t %s \t %s", "CMPI.L", "#0, D0");
  emitCode(cg->emit, line);

  sprintf(line, "\t %s \t %s", "BEQ", exitlabel);
  emitCode(cg->emit, line);

  cgBlock(cg, funnam, astwhile->block);

  sprintf(line, "\t %s \t %s", "BRA", startlabel);  // loop
  emitCode(cg->emit, line);

  sprintf(line, "%s:", exitlabel);
  emitCode(cg->emit, line);

  sprintf(line, "%s%s", exitlabel, ":");            // exit label
  emitCode(cg->emit, line);
}