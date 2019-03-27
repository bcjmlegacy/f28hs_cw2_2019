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

static uint64_t startT, stopT;

// =========================================================

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

void initIO()
{

    if(DEBUG) printd("Initializing I/O Devices\n", 0);

    int fd;

    if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0)
    {
        printd("Cannot open /dev/mem. Try sudo\n", 0);
        exit(1);
    }

    static volatile int val = 1024, val2, val3;

    // GPIO:
    gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase) ;
    if((int32_t)gpio == -1)
    {
        printd("mmap failed\n", 0);
        exit(1);
    }

    uint32_t res;

    // BCM 13
    int g_fSel    = 1;   // GPIO Register 1
    int g_shift   = 9;   // Bits 11-9

    // BCM 5
    int r_fSel    = 0;   // GPIO Register 0
    int r_shift   = 15;  // Bits 17-15

    if(DEBUG) printd("Setting GREEN LED to OUTPUT\n",0);

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

    if(DEBUG) printd("Setting RED LED to OUTPUT\n",0);

    asm(
        "\tLDR R1, %[gpio]\n"
        "\tADD R0, R1, %[r_fSel]\n"
        "\tLDR R1, [R0, #0]\n"
        "\tMOV R2, #0b111\n"
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

uint64_t timeInMicroseconds()
{
    struct timeval tv, tNow, tLong, tEnd;
    uint64_t now;
    gettimeofday(&tv, NULL);
    now = (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec;

    return (uint64_t)now;
}

void timer_handler (int signum)
{
    static int count = 0;
    stopT = timeInMicroseconds();
    count++;
    fprintf(stderr, "timer expired %d times; (measured interval %f sec)\n", count, (stopT-startT)/1000000.0);
    startT = timeInMicroseconds();
}

static inline int getitimer_asm(int which, struct itimerval *curr_value)
{
    int res;

    asm(
        "\tB _bonzo105\n"
        "_bonzo105: NOP\n"
        "\tMOV R0, %[which]\n"
        "\tLDR R1, %[buffer]\n"
        "\tMOV R7, %[getitimer]\n"
        "\tSWI 0\n"
        "\tMOV %[result], R0\n"
        : [result] "=r" (res)
        : [buffer] "m" (curr_value)
        , [which] "r" (ITIMER_REAL)
        , [getitimer] "r" (GETITIMER)
        : "r0", "r1", "r7", "cc");
        
    fprintf(stderr, "ASM: getitimer has returned a value of %d \n", res);
}

static inline int setitimer_asm(int which, const struct itimerval *new_value, struct itimerval *old_value) {

    int res;

    fprintf(stderr, "ASM: calling setitimer with this struct itimerval contents: %d secs %d micro-secs \n", 
    new_value->it_interval.tv_sec, new_value->it_interval.tv_usec);

    asm(
        "\tB _bonzo104\n"
        "_bonzo104: NOP\n"
        "\tMOV R0, %[which]\n"
        "\tLDR R1, %[buffer1]\n"
        "\tLDR R2, %[buffer2]\n"
        "\tMOV R7, %[setitimer]\n"
        "\tSWI 0\n"
        "\tMOV %[result], R0\n"
        : [result] "=r" (res)
        : [buffer1] "m" (new_value)
        , [buffer2] "m" (old_value)
        , [which] "r" (ITIMER_REAL)
        , [setitimer] "r" (SETITIMER)
        : "r0", "r1", "r2", "r7", "cc");

    fprintf(stderr, "ASM: setitimer has returned a value of %d \n", res);
}

int sigaction_asm(int signum, const struct sigaction *act, struct sigaction *oldact){
    int res;

    asm(
        "\tB _bonzo67\n"
        "_bonzo67: NOP\n"
        "\tMOV R0, %[signum]\n"
        "\tLDR R1, %[buffer1]\n"
        "\tLDR R2, %[buffer2]\n"
        "\tMOV R7, %[sigaction]\n"
        "\tSWI 0\n"
        "\tMOV %[result], R0\n"
        : [result] "=r" (res)
        : [buffer1] "m" (act)
        , [buffer2] "m" (oldact)
        , [signum] "r" (signum)
        , [sigaction] "r" (SIGACTION)
        : "r0", "r1", "r2", "r7", "cc");
    fprintf(stderr, "ASM: sigaction has returned a value of %d \n", res);
}

int main(int argc, char *argv[])
{
    system("clear");
    printf("%s: F28HS Coursework 2\n", argv[0]);

    struct sigaction sa;
    struct itimerval timer;
    sigaction_asm (SIGALRM, &sa, NULL);

    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timer_handler;

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 250000;

    // timer.it_interval.tv_sec = 0;
    // timer.it_interval.tv_usec = DELAY;

    setitimer_asm(ITIMER_REAL, &timer, NULL);
    // startT = timeInMicroseconds();

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

    int code_length = 0;
    int no_colours  = 0;

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

    
    
    


    


}