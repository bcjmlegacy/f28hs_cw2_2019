// Course F28HS "Hardware-Software Interface", 2017/18

  // -----------------------------------------------------------------------------
  // constants for RPi2
  gpiobase = 0x3F200000 ;

  // -----------------------------------------------------------------------------
  // memory mapping 
  // Open the master /dev/memory device

  if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0)
    return failure (FALSE, "setup: Unable to open /dev/mem: %s\n", strerror (errno)) ;

  // GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase) ;
  if ((int32_t)gpio == -1)
    return failure (FALSE, "setup: mmap (GPIO) failed: %s\n", strerror (errno)) ;

  // -----------------------------------------------------------------------------
  // setting the mode
  *(gpio + 1) = (*(gpio + 1) & ~(07077 << 0)) | (0b001000001001 << 0) ;  // Sets bits to one = output

  // -----------------------------------------------------------------------------
  // 
  fprintf(stderr, "Initialising (all off) ...\n");
  *(gpio + 10) = 0b1011 << 10; 

  mysleep(DELAY);
  
  fprintf(stderr, "STOP  (red on) ...\n");
  *(gpio + 7) = 0b0001 << 10; 

  mysleep(DELAY);

  fprintf(stderr, "ATTENTION  (red, yellow on) ...\n");
  *(gpio + 7) = 0b0011 << 10; 

  mysleep(DELAY);

  fprintf(stderr, "GO (green on) ...\n");
  *(gpio + 10) = 0b0011 << 10; 
  *(gpio + 7)  = 0b1000 << 10; 

  mysleep(DELAY);

#ifdef WITH_BUTTON
  fprintf(stderr, "Waiting for BUTTON press (pin %d) to continue ...\n", BUTTON);
  while (1) {
    if ((*(gpio + 13 /* GPLEV0 */) & 0b100000000000000000000000000 ) != 0)
      break;
  }
#endif

  fprintf(stderr, "Reversing order ...\n");
  fprintf(stderr, "GO (green on) ...\n");
  *(gpio + 7) = 0b1000 << 10; 

  mysleep(DELAY);
  
  fprintf(stderr, "ATTENTION  (blinking yellow) ...\n");
  *(gpio + 10) = 0b1000 << 10; 
  { int i;
    for (i=0; i<5; i++) {
      *(gpio + 7) = 0b0010 << 10; 
      mysleep(DELAY/2);
      *(gpio + 10) = 0b0010 << 10; 
      mysleep(DELAY/2);
    }
  }

  // *(gpio + 7) = 0b0001 << 10;   // this shouldn't be necessary!!

  mysleep(DELAY);

  fprintf(stderr, "STOP  (red on) ...\n");
  *(gpio + 7) = 0b0001 << 10; 

  mysleep(DELAY);

  // Clean up: all OFF
  *(gpio + 10) = 0b1011 << 10;

 fprintf(stderr, "end main.\n");
}
