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
    // Unix so probably POSIX compliant and supports colour codes.
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

int DEBUG = TRUE;

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

//Clear the terminal screen of all characters.
//Unnecessary to be a separate function?
//Yes.
void CE()
{
    system("clear");
}

//This is the main method
int main(int argc, char *argv[])
{
    //Clear terminal screen
    CE();    
    
    printf("%s\n\nF28HS Coursework 2\n\n", argv[0]);

    // Check if being run in sudo
    if (geteuid () != 0)
    {
        printd("User not sudo!\n", 0);
        exit(1);
    }

    int x;
    for (x = 0; x < argc; x++)
    {
        if (strcmp(argv[x], "-d") == 0)
        {
            DEBUG = TRUE;
            printd("ACTIVE\n", 0);
            printd("UNIX? %d\n", ((UNIX) ? TRUE : FALSE));
        }
    }

    printf("\n\n[PLAYER ONE]");

    //Choosing length of code and number of colours
    int code_length = 0;
    int no_colours  = 0;

    printf("\nPlease input the length of the code: ");
    scanf("%d", &code_length);

    while(code_length < 3 || code_length > 6)
    {
        printf("The code must be between 3 and 6 long!\n");
        printf("\nPlease input the length of the code: ");
        scanf("%d", &code_length);
    }

    printf("Please input the number of colours: ");
    scanf("%d", &no_colours);

    while(no_colours < 3 || no_colours > 10) {
        printf("Choose between 3 and 10 colours!\n");
        printf("Please input the number of colours: ");
        scanf("%d", &no_colours);
    }

    if(DEBUG)
    {
        printd("Sequence length: %d\n", code_length);
    }



    //Choosing colours
    int colours[no_colours];
    int i;
    int temp;
    for(i = 0; i < no_colours; i++)
    {
        printf("Choose the colour for position %d: ", i+1);
        scanf("%d", &temp);

        while(temp < 1 || temp > 3)
        {
            printf("\nChoose a number between 1 and 3: ");
            scanf("%d", &temp);
        }
        colours[i] = temp;
    }

    //Briefly displays secret code for player one to check
    printf("\nYour secret code is: \n");
    char *getpass(const char *prompt);

    printf("[ ");

    for(i = 0; i < no_colours; i++)
    {
        printf("%d ", colours[i]);
    }

    printf("]\n\nChanging to PLAYER TWO in five seconds...\n");

    sleep(5);

    CE();
    printf("\n[PLAYER TWO]\n\nYou have 5 tries to guess the correct code.\n");

    if(DEBUG)
    {
        printd("[ ", 0);

        for(i = 0; i < no_colours; i++)
        {
            printf("%d ", colours[i]);
        }

        printf("]\n");
    }

    printf("\n");


    int turnNumber[5];
    int guess[3];

    for(i = 0; i < no_colours; i++)
    {
        printf("Enter your guess for position %d: ", i+1);
        scanf("%d", &guess[i]);
    }
}
