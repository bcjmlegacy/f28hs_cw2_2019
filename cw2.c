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

int DEBUG   = FALSE;
int BDEBUG  = FALSE;

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

static int btnCount = 0;
static volatile unsigned int gpiobase = 0x3F200000;
static volatile uint32_t *gpio;// adapted from wiringPi; only need for timing, not for the itimer itself

// =========================================================
// Timer setup

// System call codes
#define SETITIMER 104
#define GETITIMER 105
#define SIGACTION 67

#define DELAY 5

static uint64_t startT, stopT;
static bool finished = FALSE;

// =========================================================

// Function for printing debug messages
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

// Function for setting up our GPIO devices
void initIO()
{

    if(DEBUG) printd("Initializing I/O Devices...\n", 0);

    int fd;

    // Attempt to set up access to memory controlling GPIO...
    if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0)
    {
        printd("Cannot open /dev/mem! Sudo?\n", 0);
        exit(1);
    }

    static volatile int val = 1024, val2, val3;

    // mmap GPIO
    gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase) ;
    if((int32_t)gpio == -1)
    {
        printd("mmap failed!\n", 0);
        exit(1);
    }

    uint32_t res;

    // ===============================================================
    //  For each device, set the appropriate bits in the corresponding
    //  register to enable INPUT / OUTPUT mode

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
}

// Toggle the green LED on and off
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

// Toggle the red LED on and off
void toggleRed(int reg)
{
    if(DEBUG)
    {
        printd("", 0);
        printf(KRED "RED %d %s\n" KNRM, RLED, ((reg == 10) ? "OFF" : "ON"));
    }

    uint32_t res;

    asm volatile(
        "\tLDR R1, %[gpio]\n"
        "\tADD R0, R1, %[reg]\n"
        "\tMOV R2, #1\n"
        "\tMOV R1, %[rled]\n"
        "\tAND R1, #31\n"
        "\tLSL R2, R1\n"
        "\tSTR R2, [R0, #0]\n"
        "\tMOV %[result], R2\n"
        : [result] "=r" (res)
        : [rled] "r" (RLED)
        , [gpio] "m" (gpio)
        , [reg] "r" (reg*4)
        : "r0", "r1", "r2", "cc");
}

uint64_t timeInMicroseconds()
{
    struct timeval tv, tNow, tLong, tEnd ;
    uint64_t now;
    gettimeofday(&tv, NULL);
    now = (uint64_t)tv.tv_sec * (uint64_t)100000 + (uint64_t)tv.tv_usec;
    return (uint64_t)now;
}

// This method runs when the SIGALRM interupt is raised
void timer_handler (int signum)
{
    // if(DEBUG) printf(KYEL "0" KNRM);
    finished = TRUE;
    stopT = timeInMicroseconds();
    startT = timeInMicroseconds();
}

// Checks if the button has been pressed. Increments a counter
//  if so.
void checkBtn()
{
    int reg = 13;
    int res = 0;


    // Very bad inline ASM attempt
    // asm volatile(
    //     "\tLDR R1, %[gpio]\n"
    //     "\tADD R0, R1, %[reg]\n"
    //     "\tMOV R2, #1\n"
    //     "\tMOV R1, %[button]\n"
    //     "\tAND R1, #31\n"
    //     "\tLSL R2, R1\n"
    //     "\tSTR R2, [R0, #0]\n"
    //     "\tMOV %[result], R2\n"
    //     : [result] "=r" (res)
    //     : [button] "r" (BUTTON)
    //     , [gpio] "m" (gpio)
    //     , [reg] "r" (reg*4)
    //     : "r0", "r1", "r2", "cc");

    // printf("%d\n", res);


    // We cheated with the C implementation because we struggled for time.
    if ((*(gpio + 13 /*GPLEV0*/) & (1 << (BUTTON &31))) != 0)
    {
        btnCount++;                                     // increment counter
        if(BDEBUG) printd("\n%d BUTTON", btnCount);     // print handy debug message
        fflush(stdout);                                 // flush output since we don't use a newline

        // Wait until the button stops being pressed to avoid multiple readings
        while((*(gpio + 13 /*GPLEV0*/) & (1 << (BUTTON &31))) != 0) {}  
    }
}

// Function handles getting input from the button and its timing
int getInput()
{
    static int answer = 0;
    struct sigaction sa;
    struct itimerval timer;

    // Reset button counter
    btnCount = 0;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler; // Link our signal handler
    sigaction (SIGALRM, &sa, NULL);

    // Set up our timer
    timer.it_value.tv_sec = DELAY;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = DELAY;
    timer.it_interval.tv_usec = 0;
    setitimer (ITIMER_REAL, &timer, NULL);
    startT = timeInMicroseconds();

    // Finished once the timer runs out
    finished = FALSE;

    // While the timer is still going, check for input
    while(finished == FALSE)    {
        checkBtn();
    }

    // The answer is the no. of button pressed during that time
    answer = btnCount;

    return answer;
}

// Blink the green LED a given number of times
void blinkGreen(int times)
{
    struct timespec tim, tim2;
    tim.tv_sec  = 0;
    tim.tv_nsec = 200000000;

    for(int i = 0; i < times; i++)
    {
        toggleGreen(ON);
        nanosleep(&tim , &tim2);
        toggleGreen(OFF);
        nanosleep(&tim , &tim2);
    }
}

// Blink the red LED the given number of times
void blinkRed(int times)
{
    struct timespec tim, tim2;
    tim.tv_sec  = 0;
    tim.tv_nsec = 200000000;

    for(int i = 0; i < times; i++)
    {
        toggleRed(ON);
        nanosleep(&tim , &tim2);
        toggleRed(OFF);
        nanosleep(&tim , &tim2);
    }
}

// Our main, where all the fun stuff happens
int main(int argc, char *argv[])
{
    int secret_length   = 0;    // The no. of digits in our secret
    int no_colours      = 0;    // The no. of colours we will be using

    system("clear");    // Tidy the screen

    // Check if being run in sudo
    if (geteuid () != 0)
    {
        // If not, exit
        printd("User not sudo!\n", 0);
        exit(1);
    }

    // Check the arguments for flags
    //  -d      Print debug messages
    //  -b      Print button debug messages
    //          (moved to an extra flag because they are super annoying)
    for (int x = 0; x < argc; x++)
    {
        if (strcmp(argv[x], "-d") == 0)
        {
            DEBUG = TRUE;
            printd("ACTIVE\n", 0);
            printd("UNIX? %d\n", ((UNIX) ? TRUE : FALSE));
        } 
        else
        {
            if (strcmp(argv[x], "-b") == 0)
            {  
                BDEBUG = TRUE;
                printd("BUTTON PRINTING ACTIVE\n", 0);
            }
        }   
    }

    initIO();           // Initiate our GPIO devices
    toggleRed(OFF);     // Make sure the LEDs are cleared / OFF
    toggleGreen(OFF);   

    blinkGreen(1);      // Blink both of our LEDs once to test 
    blinkRed(1);        //  they are connected properly

    printf("F28HS Coursework 2\n\n");

    printf("Please input the length of the code [3 - 10]: ");
    fflush(stdout);
    secret_length = getInput();
    printf("%d\n", secret_length);

    while(secret_length < 3 || secret_length > 10) {
        printf("The code must be between 3 and 10 digits long!\n");
        printf("Please input the length of the code [3 - 10]: ");
        fflush(stdout);
        secret_length = getInput();
        printf("%d\n", secret_length);
    }

    blinkRed(1);

    printf("Please input the number of colours [3 - 10]: ");
    fflush(stdout);
    no_colours = getInput();
    printf("%d\n", no_colours);

    while(no_colours < 3 || no_colours > 10) {
        printf("Choose between 3 and 10 colours!\n");
        printf("Please input the number of colours [3 - 10]: ");
        fflush(stdout);
        no_colours = getInput();
        printf("%d\n", no_colours);
    }

    blinkRed(1);

    // ============================================================
    //  Choosing secret code

    int secret[no_colours]; // Where we will store the secret code
    int i;                  // Loop counter
    int temp;               // Holds the entered secret digit

    // For every element of the array, request a digit
    for(i = 0; i < secret_length; i++)
    {
        printf("Choose the colour for position %d: ", i+1);
        fflush(stdout);
        temp = getInput();
        printf("%d\n", temp);

        // If the digit is outwith the given range, ask for another
        while(temp < 1 || temp > no_colours)
        {
            printf("Choose a number between 1 and %d: ", no_colours);
            fflush(stdout);
            temp = getInput();
            printf("%d\n", temp);
        }
        secret[i] = temp;
        blinkRed(1);
    }

    system("clear");    // Tidy the console again

    // Print the secret if in debug mode
    if (DEBUG) {
        printd("[ ", 0);

        for (i = 0; i < no_colours; i++) {
            printf("%d ", secret[i]);
        }

        printf("]\n");
    }

    // =========================================================================
    //  Actually trying to guess the code now

    printf("\n[PLAYER TWO]\n\nYou have 5 tries to guess the correct code.\n");

    int turnNumber;
    int guess[secret_length];

    int j;
    // Array to store which parts of the code have already
    //  been looked at so we don't count duplicate matching
    //  colors
    int checkedIndex[secret_length];    

    // Turn limit: 5.
    // Completely arbitrary.
    for (turnNumber = 0; turnNumber < 5; turnNumber++) {

        int correctColour = 0;
        int correctPlace = 0;

        // Resets checked array to zero
        for(i = 0; i < secret_length; i++){
            checkedIndex[i] = 0;
        }

        printf("\nTURN [%d]\n--------\n", turnNumber + 1);

        printf("Guess: ");
        fflush(stdout);

        for (i = 0; i < no_colours; i++)
        {
            temp = getInput();
            printf(" %d ", temp);
            fflush(stdout);
            blinkRed(1);
            blinkGreen(temp);
            guess[i] = temp;
        }

        blinkRed(2);

        // Checks if the peg is in the right place and has the right colour
        for(i = 0; i < secret_length; i++)
        {
            if(guess[i] == secret[i])
            {
                correctPlace++;
            }
        }

        // Checks if the peg is the right colour, but not necessarily in the right place
        for(i = 0; i < secret_length; i++)
        {
            for(j = 0; j < secret_length; j++)
            {
                if(secret[i] == guess[j] && checkedIndex[i] == 0)
                {
                    checkedIndex[i] = 1;
                    correctColour++;
                }
            }
        }
        printf("\nCorrect Place: %d\n",correctPlace);
        printf("Correct Colour: %d\n\n",correctColour);

        if(correctPlace == secret_length)
        {
            if(DEBUG) printd("secret_length: %d", secret_length);
            printf("\n\nYou win!\n\n");
            exit(0);
        }

        // for(i = 0; i < secret_length; i++){
        //     if(guess[i] == secret[i]){
        //         // printf("\n%d %d", guess[i], secret[i]);
        //         // printf("\n%d", winCondition);
        //         winCondition++;
        //         // printf(" %d", winCondition);
        //     }
        // }

    }

    printf("\n\nYou lose :(\n\n");

    printf("Secret code:  ");
    for(i = 0; i < no_colours; i++)
    {
        printf("%d  ", secret[i]);
    }

    exit(0);

}