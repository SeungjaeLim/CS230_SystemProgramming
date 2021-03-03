/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* Copyright (C) 1991-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/* We do not support C11 <threads.h>.  */
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  return ~(x&y)&~(~x&~y);
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
  int n8 = n<<3;
  int m8 = m<<3;
  int lifem8 = 0xff << m8;
  int lifen8 = 0xff << n8;
  int xmtmp = x & (lifem8);
  int xntmp = x & (lifen8);
  x = x & ~(lifem8);
  x = x & ~(lifen8);
  xmtmp = (xmtmp>>m8) & 0xff;
  xntmp = (xntmp>>n8) & 0xff;
  xmtmp = xmtmp << n8;
  xntmp = xntmp << m8;
  x = x | xmtmp;
  x = x | xntmp;
    return x;
}
/* 
 * rotateLeft - Rotate x to the left by nf
 *   Can assume that 0 <= n <= 31
 *   Examples: rotateLeft(0x87654321,4) = 0x76543218
 *   Legal ops: ~ & ^ | + << >> !
 *   Max ops: 25
 *   Rating: 3 
 */
int rotateLeft(int x, int n) {
  int cn = ~n+1;
  int zezero = ~0x0<<(32+cn);
  int zeroro = ~(~0x0<<n);
  int xtmp = (x&zezero)>>(32+cn);

  xtmp = xtmp & (zeroro);
  x = x<<n;
  x = x | xtmp;
  return x;
}
/*
 * leftBitCount - returns count of number of consective 1's in
 *     left-hand (most significant) end of word.
 *   Examples: leftBitCount(-1) = 32, leftBitCount(0xFFF0F0F0) = 12
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 50
 *   Rating: 4
 */
int leftBitCount(int x) {
  int n = !(~x);
  int tmp = x;
  int partition = 0;
  int chbool = 0;
  int range = 0;
  int mask = (~0)<<16;

  partition = ((tmp&mask)+(~mask)+1);
  chbool = !partition << 4;
  tmp = (tmp<<(chbool));
  n = n + (chbool);
  mask = mask << 8;

  partition = ((tmp&mask)+(~mask)+1);
  chbool = !partition << 3;
  tmp = (tmp<<(chbool));
  n = n + (chbool);
  mask = mask << 4;

  partition = ((tmp&mask)+(~mask)+1);
  chbool = !partition << 2;
  tmp = (tmp<<(chbool));
  n = n + (chbool);
  mask = mask << 2;

  partition = ((tmp&mask)+(~mask)+1);
  chbool = !partition << 1;
  tmp = tmp<<(chbool);
  n = n + (chbool);
  mask = mask << 1;

  partition = ((tmp&mask)+(~mask)+1);
  chbool = !partition;
  n = n + (chbool);

  return n;
}
/* 
 * absVal - absolute value of x
 *   Example: absVal(-1) = 1.
 *   You may assume -TMax <= x <= TMax
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 10
 *   Rating: 4
 */
int absVal(int x) {
  int ppnn = x>>31;
  return (ppnn + x)^(ppnn);
}
/* 
 * TMax - return maximum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmax(void) {
  return ~(0x1<<31);
}
/* 
 * fitsShort - return 1 if x can be represented as a 
 *   16-bit, two's complement integer.
 *   Examples: fitsShort(33000) = 0, fitsShort(-32768) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 1
 */
int fitsShort(int x) {
  int del = (x<<16)>>16; //express + : 0000 0xxx - : 1111 1xxx+1 
  int aftfit = del ^ x;
  return !aftfit; //0000 1xxx -> 1111 1xxx, 
}
/* 
 * rempwr2 - Compute x%(2^n), for 0 <= n <= 30
 *   Negative arguments should yield negative remainders
 *   Examples: rempwr2(15,2) = 3, rempwr2(-35,3) = -3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int rempwr2(int x, int n) {
  int sign = x>>31;
  int pone = !(sign); //+ : 1, -:0
  int deler = ~(~0<<n);
  int pos = x & deler;
  int n2 = (!(!pos))<<n;
  int cn2 = ~n2 +1;
  int isneg = cn2 & sign;
    return pos + isneg;
}
/* 
 * sign - return 1 if positive, 0 if zero, and -1 if negative
 *  Examples: sign(130) = 1
 *            sign(-23) = -1
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 10
 *  Rating: 2
 */
int sign(int x) {
   int mbool = !(!x);
   int tmp = x>>31;
    return mbool|tmp;
}
/* 
 * isNonNegative - return 1 if x >= 0, return 0 otherwise 
 *   Example: isNonNegative(-1) = 0.  isNonNegative(0) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 3
 */
int isNonNegative(int x) {
  return !(x>>31);
}
/* 
 * isGreater - if x > y  then return 1, else return 0 
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {
  int sx = x>>31;
  int sy = y>>31;
  int cx = ~x+1;
  int sum = y + cx; //y - x  
  int ssum = (sum>>31)&1; //y>=0 -> 0 -> 
  int seq = !((sx) ^ (sy)); //same 1 diff 0
  return ((seq)&(ssum))|((!seq)&(sy&1));
}
/* howManyBits - return the minimum number o./f bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  int partition;
  int smove = x>>1;
  int tmp = (x & ~smove) | (~x & smove);
  int chbool = 0;
  int mask = (~0)<<16;
  int n = !(tmp);
  int cn = 0;

  tmp = ~tmp;
  partition = ((tmp&mask)+(~mask)+1);
  chbool = !partition << 4;
  tmp = (tmp<<(chbool));
  n = n + (chbool);
  mask = mask << 8;

  partition = ((tmp&mask)+(~mask)+1);
  chbool = !partition << 3;
  tmp = (tmp<<(chbool));
  n = n + (chbool);
  mask = mask << 4;

  partition = ((tmp&mask)+(~mask)+1);
  chbool = !partition << 2;
  tmp = (tmp<<(chbool));
  n = n + (chbool);
  mask = mask << 2;

  partition = ((tmp&mask)+(~mask)+1);
  chbool = !partition << 1;
  tmp = tmp<<(chbool);
  n = n + (chbool);
  mask = mask << 1;

  partition = ((tmp&mask)+(~mask)+1);
  chbool = !partition;
  n = n + (chbool);
  
  cn = ~n + 1;

  return 33+ cn;
}
/* 
 * float_abs - Return bit-level equivalent of absolute value of f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument..
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_abs(unsigned uf) {
  unsigned absf = uf & ~(1<<31);
  int plusinf = 0x7f800000;
  if(absf > plusinf)
  {
    return uf;
  }
  else
  {
  return absf;
  }
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
  int intvalue = 0;
  int s = uf >> 31;
  int e = uf >> 23;
  int f = uf & 0x7fffff;
  int norm = 0x1<<(e-0x7f);
  e = e & 0xff;
  if( e == 0xff)
  {
    return 0x80000000u;
  }
  if (e<0x7f)
  {
    return 0x0;
  }
  e = e - 0x7f;
  intvalue = intvalue + norm;
  if(e>22 && e<31)
  {
    intvalue = intvalue + (f << (e-23));
    if(s == 1)
    {
        return -intvalue;
    }
    else
    {
      return intvalue;
    }
  }
  else if(e <= 23 && e>=0)
  {
    intvalue = intvalue + (f >> (23-e));
    if(s == 1)
    {
        return -intvalue;
    }
    else
    {
      return intvalue;
    }
  }
  else if(e>=31)
  {
    return 0x80000000u;
  }
  else
  {
  return 0x80000000u;
  }
}
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
  int e = uf >> 23;
  int s = uf >> 31;
  int f = uf & 0x7FFFFF;
  int edel = 0x7f800000;
  int sdel = 0x80000000;
  int fdel = 0x007fffff;
  int value = 0;
  int exp = uf & edel;
  int sign = uf & sdel;
  int frac = uf & fdel;
  int secbit = (uf & 0x3) >> 1;
  int firbit = (uf & 0x1);
  int havetoround = 0;
  if((secbit == 1) && (firbit == 1))
  {
    havetoround = 1;
  }
  e = e & 0xff;
  if(e == 0xff)
  {
    return uf;
  }
  else if(e == 0x0 || e == 0x01)
  {
    frac = exp + frac;
    frac = (uf & 0x00ffffff) >> 1;
    if(havetoround == 1)
    {
      frac = frac + 1;
    }
    value = frac;
  }
  else
  {
  value = (((exp - 1)) & edel) + frac;
  }
  return sign + value;
}
