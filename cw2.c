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
#include <signal.h>

// ================================
// Define TRUE / FALSE and check
// type of OS.
// NB: Realised later that this was irrelevant because we will
//      always be using an RPi2 for this code, so of course it's
//      not going to be run on Windows! But left this here because
//      we put effort into it. 

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

int DEBUG = FALSE;

// ============================================================================================
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

// ==============================
// GPIO setup

#define GLED    13
#define RLED    5
#define BUTTON  19

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

#define	INPUT   0
#define	OUTPUT	1

#define OFF     10
#define ON      7

static volatile unsigned int gpiobase = 0x3F200000;
static volatile uint32_t *gpio;// adapted from wiringPi; only need for timing, not for the itimer itself

// =========================================================
// Timer setup

// System call codes
#define SETITIMER 104
#define GETITIMER 105
#define SIGACTION 67

#define DELAY 3

static uint64_t startT, stopT;
static bool finished = FALSE;

// =========================================================


void printd(char* msg, int var)
{
    if(UNIX)  // If the OS is Unix, print using colours
    {
        printf(KYEL);
        printf("=debug=: ");
        printf(msg, var);
        printf(KNRM);
    }
    else
    {
        printf("=debug=: ");
        printf(msg, var);
    }
}

void initIO()
{

    if(DEBUG) printd("Initializing I/O Devices...\n", 0);

    int fd;

    if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0)
    {
        printd("Cannot open /dev/mem! Sudo?\n", 0);
        exit(1);
    }

    static volatile int val = 1024, val2, val3;

    // GPIO:
    gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase) ;
    if((int32_t)gpio == -1)
    {
        printd("mmap failed!\n", 0);
        exit(1);
    }

    uint32_t res;

    // BCM 13
    int g_fSel  = 1;   // GPIO Register 1
    int g_shift = 9;   // Bits 11-9

    // BCM 5
    int r_fSel  = 0;   // GPIO Register 0
    int r_shift = 15;  // Bits 17-15

    int b_fSel  = 1;   // GPIO Register 1
    int b_shift = 27;  // Bits 27-29

    if(DEBUG) printd("...setting GREEN LED to OUTPUT\n",0);

    asm(
        "\tLDR R1, %[gpio]\n"
        "\tADD R0, R1, %[g_fSel]\n"
        "\tLDR R1, [R0, #0]\n"
        "\tMOV R2, #0b111\n"
        "\tLSL R2, %[g_shift]\n"
        "\tBIC R1, R1, R2\n"
        "\tMOV R2, #1\n"
        "\tLSL R2, %[g_shift]\n"
        "\tORR R1, R2\n"
        "\tSTR R1, [R0, #0]\n"
        "\tMOV %[result], R1\n"
        : [result] "=r" (res)
        : [gled] "r" (GLED)
        , [gpio] "m" (gpio)
        , [g_fSel] "r" (g_fSel*4)
        , [g_shift] "r" (g_shift)
        : "r0", "r1", "r2", "cc");

    if(DEBUG) printd("...setting RED LED to OUTPUT\n",0);

    asm(
        "\tLDR R1, %[gpio]\n"
        "\tADD R0, R1, %[b_fSel]\n"
        "\tLDR R1, [R0, #0]\n"
        "\tMOV R2, #0b111\n"
        "\tLSL R2, %[b_shift]\n"
        "\tBIC R1, R1, R2\n"
        "\tMOV R2, #1\n"
        "\tLSL R2, %[b_shift]\n"
        "\tORR R1, R2\n"
        "\tSTR R1, [R0, #0]\n"
        "\tMOV %[result], R1\n"
        : [result] "=r" (res)
        : [button] "r" (BUTTON)
        , [gpio] "m" (gpio)
        , [b_fSel] "r" (b_fSel*4)
        , [b_shift] "r" (b_shift)
        : "r0", "r1", "r2", "cc");

    if(DEBUG) printd("...setting BUTTON to INPUT\n",0);

    asm(
        "\tLDR R1, %[gpio]\n"
        "\tADD R0, R1, %[r_fSel]\n"
        "\tLDR R1, [R0, #0]\n"
        "\tMOV R2, #0b000\n"
        "\tLSL R2, %[r_shift]\n"
        "\tBIC R1, R1, R2\n"
        "\tMOV R2, #1\n"
        "\tLSL R2, %[r_shift]\n"
        "\tORR R1, R2\n"
        "\tSTR R1, [R0, #0]\n"
        "\tMOV %[result], R1\n"
        : [result] "=r" (res)
        : [rled] "r" (RLED)
        , [gpio] "m" (gpio)
        , [r_fSel] "r" (r_fSel*4)
        , [r_shift] "r" (r_shift)
        : "r0", "r1", "r2", "cc");

    asm volatile(
        "\tLDR R1, %[gpio]\n"
        "\tADD R0, R1, %[reg]\n"
        "\tMOV R2, #1\n"
        "\tMOV R1, %[button]\n"
        "\tAND R1, #31\n"
        "\tLSL R2, R1\n"
        "\tSTR R2, [R0, #0]\n"
        "\tMOV %[result], R2\n"
        : [result] "=r" (res)
        : [button] "r" (BUTTON)
        , [gpio] "m" (gpio)
        , [reg] "r" (10*4)
        : "r0", "r1", "r2", "cc");
}

void toggleGreen(int reg)
{
    if(DEBUG)
    {
	    printd("", 0);
	    printf(KGRN "GREEN %d %s\n" KNRM, GLED, ((reg == 10) ? "OFF" : "ON"));
    }

    uint32_t res;

    asm volatile(
        "\tLDR R1, %[gpio]\n"
        "\tADD R0, R1, %[reg]\n"
        "\tMOV R2, #1\n"
        "\tMOV R1, %[gled]\n"
        "\tAND R1, #31\n"
        "\tLSL R2, R1\n"
        "\tSTR R2, [R0, #0]\n"
        "\tMOV %[result], R2\n"
        : [result] "=r" (res)
        : [gled] "r" (GLED)
        , [gpio] "m" (gpio)
        , [reg] "r" (reg*4)
        : "r0", "r1", "r2", "cc");
}

void toggleRed(int reg)
{
    if(DEBUG)
    {
        printd("", 0);
        printf(KRED "RED %d %s\n" KNRM, RLED, ((reg == 10) ? "OFF" : "ON"));
    }

    uint32_t res;

    asm(
        "\tLDR R1, %[gpio]\n"
        "\tADD R0, R1, %[reg]\n"
        "\tMOV R2, #1\n"
        "\tMOV R1, %[gled]\n"
        "\tAND R1, #31\n"
        "\tLSL R2, R1\n"
        "\tSTR R2, [R0, #0]\n"
        "\tMOV %[result], R2\n"
        : [result] "=r" (res)
        : [gled] "r" (RLED)
        , [gpio] "m" (gpio)
        , [reg] "r" (reg*4)
        : "r0", "r1", "r2", "cc");
}

// adapted from wiringPi; only need for timing, not for the itimer itself
uint64_t timeInMicroseconds()
{
    struct timeval tv, tNow, tLong, tEnd ;
    uint64_t now ;
    // gettimeofday (&tNow, NULL) ;
    gettimeofday (&tv, NULL) ;
    now  = (uint64_t)tv.tv_sec * (uint64_t)100000 + (uint64_t)tv.tv_usec ; // in us
    // now  = (uint64_t)tv.tv_sec * (uint64_t)1000 + (uint64_t)(tv.tv_usec / 1000) ; // in ms

    return (uint64_t)now; // (now - epochMilli) ;
}

void timer_handler (int signum)
{
    if(DEBUG) printf(KYEL "0" KNRM);
    finished = TRUE;
    stopT = timeInMicroseconds();
    startT = timeInMicroseconds();
}

int getInput()
{
    static int answer = 0;
    struct sigaction sa;
    struct itimerval timer;

    /* Install timer_handler as the signal handler for SIGALRM. */
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timer_handler;

    sigaction (SIGALRM, &sa, NULL);

    /* Configure the timer to expire after 250 msec... */
    timer.it_value.tv_sec = DELAY;
    timer.it_value.tv_usec = 0;
    /* ... and every 250 msec after that. */
    timer.it_interval.tv_sec = DELAY;
    timer.it_interval.tv_usec = 0;
    /* Start a virtual timer. It counts down whenever this process is executing. */
    // ORIG: setitimer (ITIMER_VIRTUAL, &timer, NULL);
    setitimer (ITIMER_REAL, &timer, NULL);

    /* Do busy work. */
    startT = timeInMicroseconds();
    finished = FALSE;

    while(finished == FALSE)    {
        scanf("%d", &answer);
    }

    if(answer == 0) {
        getInput();
    }

    return answer;
}

void checkBtn()
{

}

int main(int argc, char *argv[])
{
    int code_length = 0;
    int no_colours  = 0;
    int chosenColour = 0;

    system("clear");

    // printf("%s\n\nF28HS Coursework 2\n\n", argv[0]);
    printf("F28HS Coursework 2\n\n");

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

    printf("\nPlease input the length of the code: ");
    scanf("%d", &code_length);

    while(code_length < 3 || code_length> 10) {
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

    if(DEBUG) printd("Sequence length: %d\n", code_length);

    initIO();
    toggleGreen(OFF);
    toggleRed(OFF);

    printf("Please input your selection: ");

    chosenColour = getInput();

    printf("\n\nOPTION CHOSEN WAS: %d\n", chosenColour);


}