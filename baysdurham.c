/* This LFSR comes from the following website:          */
/* http://www.xilinx.com/support/documentation/         */
/* application_notes/xapp052.pdf                        */

/* This program uses an array of 32  5-bit LFSRs          */
/* and a 1024 element bit array                           */
/* The period length of a 5-bit LFSR is 31                */
/* The period length of 32 5-bit LFSRs is 1023            */
/* The period length of the 1024 element bit array        */
/* with a Bays-Durham shuffling is the factorial of 1024  */
/* 1024! is approximately 5.24e+2639                      */
/* This program runs until the bit array matches the      */
/* original bit array.                                    */
/* There is a 50% chance that the bit array will match    */
/* on the first cycle.                                    */
/* If that happens, re-run the program, until the program */
/* runs continuously.                                     */
/* Run this program continuously for at least 24 hours    */
/* to see how many cycles it processes without matching   */
/* the original bit array.                                */

/* press 'q' to exit */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <curses.h>
#include <gsl/gsl_rng.h>

#define STATES 32
/* size of the shuffle array */
#define SHUFFLE 1024

/* initialize curses */
void initcrss()
   {
   initscr();
   cbreak();
   noecho();
   nonl();
   intrflush(stdscr,FALSE);
   nodelay(stdscr,TRUE);
   } /* initcrss */

int main()
   {
   int i;                      /* cycle counter for display */
   int rslt;                   /* boolean compare result */
   int bit1;                   /* first tap bit */
   int bit2;                   /* second tap bit */
   int shift;                  /* LFSR shifted one bit to right */
   int ofst;                   /* index into state array */
   int bitofst;                /* index into bit array */
   int ch;                     /* keyboard character */
   double cycle;               /* generator cycle number */
   unsigned int dttk;          /* used to initialize taus2 */
   unsigned int *stp,*stq;     /* pointers to the state array */
   unsigned int *sdp;          /* pointer to the seed array */
   unsigned int *seed;         /* original LFSR state array */
   unsigned int *state;        /* LFSR state array */
   char *p,*q,*rp;             /* pointers into the bit array */
   char *bit;                  /* bit array */
   char *seedbit;              /* original bit array */
   time_t now;                 /* current date and time */
   clock_t clk;                /* current number of ticks */
   struct tms t;               /* structure used by times() */
   gsl_rng *r;                 /* GSL taus2 generator */
   /* define the GSL random number generator as taus2 */
   r = (gsl_rng *) gsl_rng_alloc(gsl_rng_taus2);
   /* get clock ticks since boot                           */
   clk = times(&t);
   /* get date & time                                      */
   time(&now);
   /* combine date, time, and ticks into a single UINT     */
   dttk = (unsigned int) (now ^ clk);
   /* initialize the GSL taus2 random number generator      */
   /* to date,time,#ticks                                  */
   gsl_rng_set(r, dttk);
   /********************************************************/
   /* allocate memory for the state array                  */
   /********************************************************/
   state = (unsigned int *) malloc(sizeof(unsigned int) * 64);
   if (state == NULL)
      {
      fprintf(stderr,"main: out of memory "
         "allocating *state\n");
      exit(1);
      } /* out of mem */
   /********************************************************/
   /* allocate memory for the original state array         */
   /********************************************************/
   seed = (unsigned int *) malloc(sizeof(unsigned int) * 64);
   if (seed == NULL)
      {
      fprintf(stderr,"main: out of memory "
         "allocating *seed\n");
      exit(1);
      } /* out of mem */
   /********************************************************/
   /* allocate memory for the bit array                    */
   /********************************************************/
   bit = (char *) malloc(sizeof(char) * 2048);
   if (bit == NULL)
      {
      fprintf(stderr,"main: out of memory "
         "allocating *bit\n");
      exit(1);
      } /* out of mem */
   /********************************************************/
   /* allocate memory for the original bit array           */
   /********************************************************/
   seedbit = (char *) malloc(sizeof(char) * 2048);
   if (seedbit == NULL)
      {
      fprintf(stderr,"main: out of memory "
         "allocating *seedbit\n");
      exit(1);
      } /* out of mem */
   /********************************************************/
   /* Initialize the bit array                             */
   /********************************************************/
   p = (char *) bit;
   q = (char *) bit + SHUFFLE;
   rp = (char *) seedbit;
   while (p < q)
      {
      *rp++ = *p++ = (char) gsl_rng_uniform_int(r, 2);   /* 0,1 */
      } /* for each bit */
   /********************************************************/
   /* Initialize the state array                           */
   /********************************************************/
   stp = (unsigned int *) state;
   stq = (unsigned int *) state + STATES;
   sdp = (unsigned int *) seed;
   while (stp < stq)
      {
      *stp = (unsigned int) gsl_rng_uniform_int(r, STATES);  /* 0-31 */
      if (*stp != 0)
	 {
	 *sdp = *stp;
         stp++;
	 sdp++;
	 } /* if non-zero */
      } /* for each 5-bit state */
   /********************************************************/
   /* start curses mode                                    */
   /********************************************************/

   initcrss();             /* initialize curses mode */

   /********************************************************/
   /* main generator loop                                  */
   /********************************************************/
   cycle = 0.0;
   i = 0;
   while (1)
      {
      int out;      /* output of the generator          */
      int tmp;      /* used for shuffling the bit array */
      cycle += 1.0;
      i++;
      /* this is 5 3 reversed for a 5-bit LFSR */
      /* this version is counted from the left at zero */
      /* in the reversed version... */
      /* the bits are counted from the right at zero */
      /* the number of cycles are correct at 31 */
      /* using a random 5 bit number for a seed */
      /* 2 0 */
      ofst = (int) gsl_rng_uniform_int(r, STATES);  /* 0-31 */
      stp = (unsigned int *) state + ofst;;
      /* xor the LFSR taps to create an output bit */
      bit1 = *stp >> 2 & 1;
      bit2 = *stp & 1;
      out = bit1 ^ bit2;
      /* roll the LFSR right using the output bit as carry */
      shift = (*stp >> 1) & 0x0f;      /* 5-bit LFSR */
      *stp = (shift | (out << 4)) & 0x1f;
      /*****************************************************/
      /* Bays-Durham shuffle                               */
      /* 1024 bit shuffle:  1024! = 5.41e+2639 base 10     */
      /*****************************************************/
      /* calculate a random offset into the bit array      */
      /*****************************************************/
      bitofst = (int) gsl_rng_uniform_int(r, SHUFFLE);  /* 0-1023 */
      p = (char *) bit + bitofst;
      /* swap the current output with the chosen element   */
      /* in the bit array.                                 */
      tmp = *p;
      *p  = out;
      out = tmp;
      /*************************************************/
      /* Compare the bit array to the original array   */
      /*************************************************/
      rslt = 0;   /* initialize boolean result of compare */
      p = (char *) bit;
      q = (char *) bit + SHUFFLE;
      rp = (char *) seedbit;
      while (p < q)
         {
	 /* bounce out if mis-match */
	 if (*p != *rp)
	    {
	    rslt = 1;
	    break;
	    } /* if mis-match */
	 p++;
	 rp++;
	 } /* for each bit in bit array */
      /* if current bit array matches the original bit array */
      /* stop program */
      if (!rslt)
         {
	 break;
	 } /* if bit array matched */
      if (i >= 100000000)         /* one hundred million */
         {
	 char str[128];
         move(5,28);
         sprintf(str,"Bays-Durham Shuffle");
         addstr(str);
	 move(10,25);
	 sprintf(str,"%9.1f Billion", cycle / 1000000000.0);
	 addstr(str);
         ch = getch();
         if (ch == 'q') break;
	 i = 0;
	 } /* every million cycles */
      } /* for each cycle */
   endwin();           /* de-activate curses mode */
   if (rslt)
      {
      printf("bit array has not matched "
         "as of cycle %9.0f Million\n", cycle / 1000000.0);
      } /* if mis-match */
   else
      {
      printf("bit array has matched on cycle %.0f\n", cycle);
      } /* if mis-match */
   gsl_rng_free(r);
   return(0);
   } /* main */
