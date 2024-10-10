/*
 * measure the time it takes to load an LNG file
 * Copyright (C) 2024 Mateusz Viste
 */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h> /* outp() */

#include "../svarlang.h"


static unsigned long nowtime = 0;        /* current time counter */
static void interrupt (*oldfunc)(void);  /* interrupt function pointer */



static void interrupt handle_clock(void) {
  static int callmod = 0;

  /* increment the time */
  nowtime += 858; /* one cycle is 858.21us to be exact */

  /* increment the callmod */
  callmod++;
  callmod &= 63; /* call the original clock INT every 64 calls */

  /* if this is the 64th call, then call handler */
  if (callmod == 0) {
    nowtime += 13; /* compensate for integer division inaccuracy */
    _chain_intr(oldfunc);
  } else {  /* otherwise, clear the interrupt controller */
    outp(0x20, 0x20);  /* end of interrupt */
  }
}


/* This routine will stop the timer. It has void return value so that it
 * can be an exit procedure. */
static void timer_stop(void) {
  /* Disable interrupts */
  _disable();

  /* Reinstate the old interrupt handler */
  _dos_setvect(0x08, oldfunc);

  /* Reinstate the clock rate to standard 18.2 Hz */
  outp(0x43, 0x36);       /* Set up for count to be sent          */
  outp(0x40, 0x00);       /* LSB = 00  \_together make 65536 (0)  */
  outp(0x40, 0x00);       /* MSB = 00  /                          */

  /* Enable interrupts */
  _enable();
}


/* This routine will start the fast clock rate by installing the handle_clock
 * routine as the interrupt service routine for the clock interrupt and then
 * setting the interrupt rate up to its higher speed by programming the 8253
 * timer chip. */
static void timer_init(void) {
  /* Store the old interrupt handler */
  oldfunc = _dos_getvect(0x08);

  /* Set the nowtime to zero */
  nowtime = 0;

  /* Disable interrupts */
  _disable();

  /* Install the new interrupt handler */
  _dos_setvect(0x08, handle_clock);

  /* Increase the clock rate */
  outp(0x43, 0x36);     /* Set up for count to be sent            */
  outp(0x40, 0x00);     /* LSB = 00  \_together make 2^10 = 1024  */
  outp(0x40, 0x04);     /* MSB = 04  /                            */

  /* Enable interrupts */
  _enable();
}


int main(int argc, char **argv) {
  int r;

  if (argc != 2) {
    puts("loads lang from FDISK.LNG and measure how long it took");
    puts("usage: tim lang (example: tim PL)");
    return(1);
  }

  timer_init();
  r = svarlang_load("FDISK.LNG", argv[1]);
  timer_stop();

  if (r != 0) {
    printf("svarlang_load('FDISK.LNG', '%s') failed with err %d\n", argv[1], r);
    return(1);
  } else {
    int i, count = 0;
    printf("svarlang_load('FDISK.LNG', '%s') took %lu ms\nFirst 3 strings:\n", argv[1], nowtime / 1024);
    for (i = 0; i < 0xffff; i++) {
      const char *s = svarlang_strid(i);
      if (*s == 0) continue;
      puts(s);
      count++;
      if (count == 3) break;
    }
  }


  return(0);
}
