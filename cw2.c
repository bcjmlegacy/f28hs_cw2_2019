/*
    cw2.c
    F28HS Coursework 2

    Author:     Benjamin Milne & Alakbar Zeynalzade
    Inst.:      Heriot-Watt University
    ID:         H00267054 & HXXXXXXXX
    Contact:    bm56@hw.ac.uk & az40@hw.ac.uk
    Date:       March, 2019
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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


}