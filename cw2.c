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
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

typedef int bool;
#define TRUE    1
#define FALSE   0
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

//asm("\t.addr_file:	.word	.file\n"
//    "\t.data\n"
//    "\t.file:		.ascii \"/dev/mem\000\"\n");

asm("\t .global main\n"

    "\tmain:\n"
        "\tSUB	SP, SP, #16		@ Create 16 bytes storage\n"
        "\tLDR	R0, .addr_file	@ get GPIO Controller addr\n"
        "\tLDR	R1, .flags		@ set flag permissions\n"
        "\tBL	open			@ call to get file handle\n"

        "\tSTR	R0, [SP, #12]		@ File handle number\n"
        "\tLDR	R3, [SP, #12]		@ Get File handle\n"
        "\tSTR	R3, [SP, #0]		@ Store file handle on top stack\n"
        "\tLDR	R3, .gpiobase		@ get GPIO_Base address\n"
        "\tSTR	R3, [SP, #4]		@ store on SP+4\n"
        "\tMOV	R0, #0			@ R0=0\n"
        "\tMOV	R1, #4096		@ R1=page\n"
        "\tMOV	R2, #3			@ R2=3\n"
        "\tMOV	R3, #1			@ R3=1 (stdouts)\n"
        "\tBL	mmap			@ call libc fct for mmap\n"

        "\tSTR	R0, [SP, #16]		@ store virtual mem addr\n"
        "\tLDR	R3, [SP, #16]		@ get virtual mem addr\n"
    "\tfctsel:	\n"
        "\tADD	R3, R3, #16		@ add 16 for block 4 (GPFSEL4)\n"
        "\tLDR	R2, [SP, #16]		@ get virtual mem addr\n"
        "\tADD	R2, R2, #16		@ add 16 for block 4 (GPFSEL4)\n"
        "\tLDR	R2, [R2, #0]		@ load R2 with value at R2\n"
        "\tBIC	R2, R2, #0b111<<21	@ Bitwise clear of three bits\n"
        "\tSTR	R2, [R3, #0]		@ Store result in Register [set input]\n"
        "\tLDR	R3, [SP, #16]		@ Get virtual mem address\n"
        "\tADD	R3, R3, #16		@ Add 16 for block 4 (GPFSEL4)\n"
        "\tLDR	R2, [SP, #16]		@ Get virtual mem addr\n"
        "\tADD	R2, R2, #16		@ add 16 for block 4 (GPFSEL4)\n"
        "\tLDR	R2, [R2, #0]		@ Load R2 with value at R2\n"
        "\tORR	R2, R2, #1<<21		@ Set bit....\n"
        "\tSTR	R2, [R3, #0]		@ ...and make output\n"
    "\ton:	\n"
        "\tLDR	R3, [SP, #16]		@ get virt mem addr\n"
        "\tADD	R3, R3, #32		@ add 32 to offset to set register GPSET1\n"
        "\tMOV	R4, #1			@ get 1\n"
        "\tMOV	R2, R4, LSL#15		@ Shift by pin number\n"
        "\tSTR	R2, [R3, #0]		@ write to memory\n"
        "\tLDR	R0, [SP, #12]		@ get file handle\n"
        "\tBL	close			@ close file\n"

        "\tADD	SP, SP, #16		@ restore SP\n"
        
        "\tMOV R7, #1\n"
        "\tSWI 0\n"


    "\t.addr_file:	.word	.file\n"
    "\t.flags:		.word	1576962\n"
    "\t@.gpiobase:	.word	0x20200000	@ GPIO_Base for Pi 1\n"
    "\t.gpiobase:	.word	0x3F200000	@ GPIO_Base for Pi 2\n"

    "\t.data"
    "\t.file:	.ascii \"/dev/mem\000\"\n"
);

int DEBUG = FALSE;

void printd(char* msg, int var)
{
    if(UNIX)  // If the OS is Unix, print using colours
    {
        printf(KRED);
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

void greenOn()
{
    //int base = GPIOBASE;
    //char* addr = GPIOADDR;

    printd("Turning GREEN ON\n", 0);
    //asm("\tSUB SP, SP, #16\n");
    //asm("\tLDR R0, .addr_file\n");

}

int main(int argc, char *argv[])
{
    printf("%s: F28HS Coursework 2\n", argv[0]);

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

    greenOn();


}
