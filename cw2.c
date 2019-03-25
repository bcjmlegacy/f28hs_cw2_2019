/*
    cw2.c
    F28HS Coursework 2

    Authors:    Benjamin Milne & Alakbar Zeynalzade
    Inst.:      Heriot-Watt University
    ID:         H00267054 & H00274131
    Contact:    bm56@hw.ac.uk & az40@hw.ac.uk
    Date:       March, 2019
*/

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

#ifndef TRUE
    typedef int bool;
    #define TRUE    1
    #define FALSE   0
#endif

#ifdef __unix__
    // Unix so probably POSIX compliant and
    //   supports colour codes.
    #define UNIX TRUE
#else
    // Not Unix so don't know if we support
    //   colour codes. Print without them.
    #define UNIX FALSE    // Not Unix so don't use coloured terminal
#endif

// UNIX terminal colour codes
// From https://stackoverflow.com/questions/3585846/color-text-in-terminal-applications-in-unix

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define GLED    13
#define RLED    5
#define BUTTON  19

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

#define	INPUT			 0
#define	OUTPUT			 1

static volatile unsigned int gpiobase ;
static volatile uint32_t *gpio ;

int DEBUG = FALSE;

void printd(char* msg, int var)
{
    if(UNIX)  // If the OS is Unix, print using colours
    {
        printf(KYEL);
        printf("DEBUG: ");
        printf(msg, var);
        printf(KNRM);
    }
    else
    {
        printf("DEBUG: ");
        printf(msg, var);
    }
}

void initLED()
{

<<<<<<< HEAD
    if(DEBUG)
    {
	    printd("Init LED\n", 0);
    }

    int fd;

    if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0)
    {
        printd("Cannot open /dev/mem. Try sudo\n", 0);
        exit(1);
    }
=======

    static volatile int val = 1024, val2, val3;

    printd("Turning GREEN ON\n", 0);
    asm(
        "\tMOV eax, #16\n" 
        "\teax");

    asm(/* multi-line example */
      "\tMOV R0, %[value]\n"         /* load the address into R0 */ 
      "\tLDR %[result], [R0, #0]\n"  /* get and return the value at that address */
      : [result] "=r" (val3) 
      : [value] "r" (&val)
      : "eax", "cc" );
>>>>>>> 4ed7e8fa09dfec090e23deebbb739ab1d443734c

    // GPIO:
    gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase) ;
    if((int32_t)gpio == -1)
    {
        printd("mmap failed\n", 0);
        exit(1);
    }

}

void greenOn()
{
    if(DEBUG)
    {
	    printd("", 0);
	    printf(KGRN "GREEN ON\n" KNRM);
    }
}

void redOn()
{
    if(DEBUG)
    {
        printd("", 0);
        printf(KRED "RED ON\n" KNRM);
    }
}

int main(int argc, char *argv[])
{
    printf("%s: F28HS Coursework 2\n", argv[0]);

    // Check if being run in sudo
    if (geteuid () != 0)
    {
        printd("User not sudo!\n", 0);
        exit(1);
    }

    for (int x = 0; x < argc; x++)
    {
        if (strcmp(argv[x], "-d") == 0)
        {
            DEBUG = TRUE;
            printd("ACTIVE\n", 0);
            printd("UNIX? %d\n", ((UNIX) ? TRUE : FALSE));
        }
    }

    int seq_length = 0;

    printf("\nPlease input the length of the sequence: ");
    scanf("%d", &seq_length);

    if(DEBUG)
    {
        printd("Sequence length: %d\n", seq_length);
    }

    initLED();
    greenOn();
    redOn();

}
