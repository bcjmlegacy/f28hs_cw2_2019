// Blinking LED, now really standalone; LED controlled from assembler level
// Compile: gcc  -o  t33 tinkerHaWo33.c
// Run:     sudo ./t33

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

// no longer needed
// #include "wiringPi.h"

// =======================================================
// Tunables
// The OK/Act LED is connected to BCM_GPIO pin 47 (RPi 2)
#define ACT  47
// delay for blinking
#define DELAY 700
// unused
#define LED 12
#define BUTTON 16
// =======================================================

#ifndef	TRUE
#define	TRUE	(1==1)
#define	FALSE	(1==2)
#endif

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

#define	INPUT			 0
#define	OUTPUT			 1

#define	LOW			 0
#define	HIGH			 1


static volatile unsigned int gpiobase ;
static volatile uint32_t *gpio ;

// -----------------------------------------------------------------------------

int failure (int fatal, const char *message, ...)
{
  va_list argp ;
  char buffer [1024] ;

  if (!fatal) //  && wiringPiReturnCodes)
    return -1 ;

  va_start (argp, message) ;
  vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;

  fprintf (stderr, "%s", buffer) ;
  exit (EXIT_FAILURE) ;

  return 0 ;
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* HaWo: tinkering starts here */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

int main (void)
{
  int pinACT = LED; // ,  pinLED = LED, pinButton = BUTTON;
  int fSel, shift, pin, clrOff, setOff;
  int   fd ;
  int   j;
  int theValue, thePin;
  unsigned int howLong = DELAY;
  uint32_t res; /* testing only */

  printf ("Raspberry Pi blinking LED %d\n", ACT) ;

  if (geteuid () != 0)
    fprintf (stderr, "setup: Must be root. (Did you forget sudo?)\n") ;

  // -----------------------------------------------------------------------------
  // constants for RPi2
  gpiobase = 0x3F200000 ;
  // BCM2708_PERI_BASE = 0x3F000000 ;

  // -----------------------------------------------------------------------------
  // memory mapping 
  // Open the master /dev/memory device

  if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0)
    return failure (FALSE, "setup: Unable to open /dev/mem: %s\n", strerror (errno)) ;

  // GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase) ;
  if ((int32_t)gpio == -1)
    return failure (FALSE, "setup: mmap (GPIO) failed: %s\n", strerror (errno)) ;
  else
    fprintf(stderr, "NB: gpio = %x for gpiobase %x\n", gpio, gpiobase);

  // -----------------------------------------------------------------------------
  // setting the mode
  fprintf(stderr, "setting pin %d to %d ...\n", pinACT, OUTPUT);
  fSel =  1;    // GPIO 12 lives in register 1 (GPFSEL)
  shift =  6;  // GPIO 12 sits in slot 2 of register 1, thus shift by 2*3 (6 bits per pin)
  // *(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (1 << shift) ;  // Sets bits to one = output
  // // *(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) ;              // Sets bits to zero = input
  // INLINED table lookups for GPIO 47
  //fSel =  4;    // GPIO 47 lives in register 4 (GPFSEL)
  //shift =  21;  // GPIO 47 sits in slot 7 of register 4, thus shift by 7*3 (3 bits per pin)
  // *(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (1 << shift) ;  // Sets bits to one = output

  asm(/* inline assembler version of setting LED to ouput" */
      "\tB _bonzo3\n"
      "_bonzo3: NOP\n"
      "\tLDR R1, %[gpio]\n"
      "\tADD R0, R1, %[fSel]\n"  /* R0 = GPFSEL register to write to */
      "\tLDR R1, [R0, #0]\n"     /* read current value of the register */
      "\tMOV R2, #0b111\n"
      "\tLSL R2, %[shift]\n"
      "\tBIC R1, R1, R2\n"
      "\tMOV R2, #1\n"
      "\tLSL R2, %[shift]\n"
      "\tORR R1, R2\n"
      "\tSTR R1, [R0, #0]\n"
      "\tMOV %[result], R1\n"
      : [result] "=r" (res)
      : [act] "r" (pinACT)
      , [gpio] "m" (gpio)
      , [fSel] "r" (fSel*4)  /* offset in bytes! */
      , [shift] "r" (shift)
      : "r0", "r1", "r2", "cc");
  fprintf(stderr, "ASM SET GPFSEL %d at mmap'ed address %x to %d (bitmask %x) \n", fSel, (gpio+fSel), res, res);

  /* NOP example */
  /* asm("mov r0,r0"); */
  //       "LDR  R0, %[gpio]\n\t" : : [gpio] "r" (gpio) : 
  //       "LDR  R1, %[gpio]\n\t" : : [gpio] "r" (gpio) : 

#if 0
  asm(/* inline assembler version of setting LED to ouput" */
      "\tLDR R0, %[gpio]\n"
      "\tADD R0, R0, #0\n"
      "\tLDR R1, %[gpio]\n"
      "\tADD R1, R1, #0\n"
      "\tLDR R1, [R1, #0]\n"
      "\tBIC R1, R1, #0b111<<5\n"
      "\tSTR R1, [R0, #0]\n"
      : 
      : [gpio] "m" (gpio)
      : "r0", "r1", "cc");

  // too verbose: don't need to save and restore regs, if clobber info is given
  asm(
      "\tB _main0\n"
      ".gpiobase:	.word	0x3F200000\n"
      "_main0: SUB SP, SP, #16\n\t"
      "\tSTR R0, [SP, #12]\n\t"
      "\tSTR R1, [SP, #8]\n\t"
      "\tLDR  R0, %0" : : "r"(gpio)  :
      "\tADD R0, R0, #0"
      "\tLDR  R1, %0" : : "r"(gpio)  :
      "\tADD R1, R1, #0\n"
      "\tLDR R1, [R1, #0]\n"
      "\tBIC R1, R1, #0b111<<5\n"
      "\tSTR R1, [R0, #0]\n"
      "\tLDR R1, [SP, #8]\n"
      "\tLDR R0, [SP, #12]\n"
      "\tADD SP, SP, #16\n");
#endif

  // -----------------------------------------------------------------------------

  // now, start a loop, listening to pinButton and if set pressed, set pinLED
 fprintf(stderr, "starting loop ...\n");
 for (j=0; j<1000; j++)
  {
    theValue = ((j % 2) == 0) ? HIGH : LOW;

    // INLINED version of digitalWrite()
    // if (theValue==HIGH)  fprintf(stderr, "Writing %d to pin %d\n", theValue, pinACT);
    if ((pinACT & 0xFFFFFFC0 /* PI_GPIO_MASK */) == 0)	// bottom 64 pins belong to the Pi	
      {
	int off = (theValue == LOW) ? 10 : 7; // LED 12; register number for GPSET/GPCLR
	// int off = (theValue == LOW) ? 11 : 8; // ACT/LED 47; register number for GPSET/GPCLR
	uint32_t res;

#if 1
	asm volatile(/* inline assembler version of setting/clearing LED to ouput" */
	        "\tB   _bonzo0\n"
                /* "vodoo0:  .word %[gpio]\n" */
	        "_bonzo0: NOP\n"
		"\tLDR R1, %[gpio]\n"
	        "\tADD R0, R1, %[off]\n"  /* R0 = GPSET/GPCLR register to write to */
	        "\tMOV R2, #1\n"
	        "\tMOV R1, %[act]\n"
	        "\tAND R1, #31\n"
	        "\tLSL R2, R1\n"          /* R2 = bitmask setting/clearing register %[act] */
	        "\tSTR R2, [R0, #0]\n"    /* write bitmask */
		"\tMOV %[result], R2\n"
		: [result] "=r" (res)
	        : [act] "r" (pinACT)
		, [gpio] "m" (gpio)
		, [off] "r" (off*4)
	        : "r0", "r1", "r2", "cc");
        fprintf(stderr, "ASM SET GPIO register %d at mmap'ed address %x to %d (bitmask %x) \n", pinACT, (gpio+off), res, res);
#endif

#if 0
        // ASSERT:  wiringPiMode == WPI_MODE_GPIO
	// fprintf(stderr, "HaWo says: YES, value %d in WPI_MODE_GPIO (pin=%d, gpio=%x)\n", theValue, pin, gpio);
        if (theValue == LOW) { // GPCLR
	  // offset for GPIO 47 is 44
	  // clrOff = 11; // why not: 44???; // control reg 11 i.e. offset 44 for GPCLR
          // *(gpio + clrOff) = 1 << (pinACT & 31) ;
	  asm(/* inline assembler version of setting LED to ouput" */
	        "\tB   _bonzo0\n"
                ".vodoo0:  .word %[gpio]\n"
	        "_bonzo0: NOP\n"
		"\tLDR R0, .vodoo0\n"
	        /* "\tMOV R0, %[gpio]\n" */
	        "\tADD R0, R0, #11\n"
	        "\tMOV R2, #1\n"
	        "\tLDR R1, %[act]\n"
	        "\tAND R1, #31\n"
	        "\tLSL R2, R1\n"
	        "\tSTR R2, [R0, #0]\n"
	        : [act] "=r" (pinACT)
	        : [gpio] "r" (&gpio)
	        : "r0", "r1", "r2", "cc");
        } else { // GPSET
	  // offset for GPIO 47 is 32
	  // setOff = 8; // why not: 32???; // control reg 8 i.e. offset 32 for GPSET
          // *(gpio + setOff) = 1 << (pinACT & 31) ;
	  asm(/* inline assembler version of setting LED to ouput" */
	        "\tB   _bonzo1\n"
                ".vodoo1:  .word %[gpio]\n"
	        "_bonzo1: NOP\n"
		"\tLDR R0, .vodoo1\n"
	        "\tADD R0, R0, #8\n"
	        "\tMOV R2, #1\n"
	        "\tLDR R1, %[act]\n"
	        "\tAND R1, #31\n"
	        "\tLSL R2, R1\n"
	        "\tSTR R2, [R0, #0]\n"
	        : [act] "=r" (pinACT)
	        : [gpio] "r" (&gpio)
	        : "r0", "r1", "r2", "cc");
	}
#endif
      }
      else
      {
        fprintf(stderr, "only supporting on-board pins\n");          exit(1);
       }

    // INLINED delay
    {
      struct timespec sleeper, dummy ;

      // fprintf(stderr, "delaying by %d ms ...\n", howLong);
      sleeper.tv_sec  = (time_t)(howLong / 1000) ;
      sleeper.tv_nsec = (long)(howLong % 1000) * 1000000 ;

      nanosleep (&sleeper, &dummy) ;
    }
  }
 // Clean up: write LOW
 clrOff = 10; // why not: 44???; // control reg 11 i.e. offset 44 for GPCLR
 *(gpio + clrOff) = 1 << (pinACT & 31) ;

 fprintf(stderr, "end main.\n");
}
