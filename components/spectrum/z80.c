/* 
 * Copyright (C) 1996-1998 Szeredi Miklos
 * Email: mszeredi@inf.bme.hu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "z80.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

Z80 PRNM(proc);

byte PRNM(inports)[PORTNUM];
byte PRNM(outports)[PORTNUM];


#ifdef SPECT_MEM
#define NUM64KSEGS 1 // bjs, was 3, too big for odroid
#endif

#ifndef NUM64KSEGS
#define NUM64KSEGS 1
#endif

static byte *a64kmalloc(int num64ksegs)
{
  byte *bigmem;
  unsigned int x;
  char s[30];
  
  //bigmem = (byte *) malloc((unsigned) (0x10000 * (num64ksegs + 1)));
  bigmem = (byte *) malloc(90000); // bjs largest working malloc...
  //bigmem = (byte *) malloc(65536); // bjs absolute minimum allowed...
  //bigmem = (byte *) malloc(15536); // testing......
  if(bigmem == NULL) {
    fprintf(stderr, "Out of memory!\n");
    exit(1);
  }
  x=(( (long) bigmem & ((long) 0xFFFF)) );
//  print("overflow value is:");
//  sprintf(s,"%i",x); print(s);

  if (x>20000) {printf("z80.c malloc miss-alignment too high"); sleep(100000);}
  return (byte *) (( (long) bigmem & ~((long) 0xFFFF)) + 0x10000);
}



void PRNM(init)(void) 
{
  qbyte i;

  
  //DANM(mem) = a64kmalloc(NUM64KSEGS);
  DANM(mem) = a64kmalloc(0); // bjs test
  srand((unsigned int) time(NULL));
  for(i = 0; i < 0x10000; i++) DANM(mem)[i] = (byte) rand();
  //for(i = 0; i < 0x10000; i++) DANM(mem)[i] = (byte)255;

  for(i = 0; i < NUMDREGS; i++) {
    DANM(nr)[i].p = DANM(mem);
    DANM(nr)[i].d.d = (dbyte) rand();
  }

  for(i = 0; i < BACKDREGS; i++) {
    DANM(br)[i].p = DANM(mem);
    DANM(br)[i].d.d = (dbyte) rand();
  }

  for(i = 0; i < PORTNUM; i++) PRNM(inports)[i] = PRNM(outports)[i] = 0;

  PRNM(local_init)();

  return;
}

/* TODO: no interrupt immediately afer EI (not important for spectrum) */

void PRNM(nmi)(void)
{
  DANM(iff2) = DANM(iff1);
  DANM(iff1) = 0;

  DANM(haltstate) = 0;
  PRNM(pushpc)();

  PC = 0x0066;
}

/* TODO: IM 0 emulation */

void PRNM(interrupt)(int data)
{
  if(DANM(iff1)) {

    DANM(haltstate) = 0;
    DANM(iff1) = DANM(iff2) = 0;

    switch(DANM(it_mode)) {
    case 0:
      PRNM(pushpc)();
      PC = 0x0038;
      break;
    case 1:
      PRNM(pushpc)();
      PC = 0x0038;
      break;
    case 2:
      PRNM(pushpc)();
      PCL = DANM(mem)[(dbyte) (((int) RI << 8) + (data & 0xFF))];
      PCH = DANM(mem)[(dbyte) (((int) RI << 8) + (data & 0xFF) + 1)];
      break;
    }
  }
}


void PRNM(reset)(void)
{
  DANM(haltstate) = 0;
  DANM(iff1) = DANM(iff2) = 0;
  DANM(it_mode) = 0;
  RI = 0;
  RR = 0;
  PC = 0;
}
