/* This LFSR comes from the following website:          */
/* http://www.xilinx.com/support/documentation/         */
/* application_notes/xapp052.pdf                        */

/* I do not have a suitable reference, yet, in the literature */
/* for shuffling.                                             */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <gsl/gsl_rng.h>

#define STATES 32
#define SHUFFLE 1024

int main()
   {
   int rslt;                   /* boolean compare result */
   int bit1;                   /* first tap bit */
   int bit2;                   /* second tap bit */
   int shift;                  /* LFSR shifted one bit to right */
   int ofst;                   /* index into state array */
   int bitofst;                /* index into bit array */
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
      *p = (char) gsl_rng_uniform_int(r, 2);     /* 0,1 */
      *rp = *p;
      p++;
      rp++;
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
   /* main generator loop                                  */
   /********************************************************/
   cycle = 0.0;
   printf("    cycle  ofst  old  bit2  bit0  "
      "XOR   shift  new   out\n");
   while (1)
      {
      int out;      /* output of the generator          */
      int tmp;      /* used for shuffling the bit array */
      cycle += 1.0;
      /* this is 5 3 reversed for a 5-bit LFSR */
      /* this version is counted from the left at zero */
      /* in the reversed version... */
      /* the bits are counted from the right at zero */
      /* the number of cycles are correct at 31 */
      /* using a random 5 bit number for a seed */
      /* 2 0 */
      ofst = (int) gsl_rng_uniform_int(r, STATES);  /* 0-31 */
      stp = (unsigned int *) state + ofst;;
      bit1 = *stp >> 2 & 1;
      bit2 = *stp & 1;
      out = bit1 ^ bit2;
      shift = (*stp >> 1) & 0x0f;      /* 5-bit LFSR */
      printf("%9.0f   %2d    %02x   %d     %d"
         "     %d      %02x  ",
	 cycle, ofst, *stp, bit1, bit2, out, shift);
      *stp = (shift | (out << 4)) & 0x1f;
      printf("  %02x  ", *stp);
      /******************************************/
      /* this block is for shuffling            */
      /* reference to shuffling goes here       */
      /******************************************/
      /* 32   bit shuffle:    32! = 2.63e35   base 10      */
      /* 1024 bit shuffle:  1024! = 5.41e2639 base 10      */
      /* In the eegl RNG, this call is not made separately */
      bitofst = (int) gsl_rng_uniform_int(r, SHUFFLE);  /* 0-1023 */
      p = (char *) bit + bitofst;
      tmp = *p;
      *p  = out;
      out = tmp;
      /******************************************/
      printf("  %d\n", out);
      /********************************************************/
      /* check for state array wrap-around                    */
      /* This check is incidental to the period length        */
      /* of the bit array                                     */
      /********************************************************/
      sdp = (unsigned int *) seed + ofst;
      if (*stp == *sdp)
         {
         printf("duplicate LFSR %02x at ofst %d cycle %.0f\n",
            *stp, ofst, cycle);
	 } /* if duplicate state */ 
      /*************************************************/
      /* Compare the bit array to it's original state  */
      /*************************************************/
      rslt = 0;
      p = (char *) bit;
      q = (char *) bit + SHUFFLE;
      rp = (char *) seedbit;
      while (p < q)
         {
	 if (*p != *rp)
	    {
	    rslt = 1;
	    break;
	    } /* if mis-match */
	 p++;
	 rp++;
	 } /* for each bit in bit array */
      if (!rslt)
         {
	 break;
	 } /* if bit array matched */
      } /* for each cycle */
   printf("bit array has match on cycle %.0f\n", cycle);
   gsl_rng_free(r);
   return(0);
   } /* main */
