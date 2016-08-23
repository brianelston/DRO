/*
 *  Mersenne.c
 *  DRO
 *
 *  Copyright 2011 UWMC Radiology IRL - University of Washington Nuclear Medicine. All rights reserved.
 *	Author: Brian Elston - bfe, S. Gillespie
 *	Contributors: R. Harrison, P. Kinahan, T. Lewellen
 
 1.20.9		bfe		Initial pass.
 8.8.11		bfe		Integration into DRO.
 *
 */

#include "Constants.h"
#include "Mersenne.h"

MersenneTwister::MersenneTwister()
{
	left = 1;
	initf = 0;
	MTRand_IsInited = false;
}
//--------------------------------------------------------
/* initializes state[N] with a seed */
void MersenneTwister::init_genrand(unsigned long s)
{
    int j;
    state[0]= s & 0xffffffffUL;
    for (j=1; j<N; j++) {
        state[j] = (1812433253UL * (state[j-1] ^ (state[j-1] >> 30)) + j); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array state[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        state[j] &= 0xffffffffUL;  /* for >32 bit machines */
    }
    left = 1; initf = 1;
}
//--------------------------------------------------------
void MersenneTwister::next_state(void)
{
    unsigned long *p=state;
    int j;

    /* if init_genrand() has not been called, */
    /* a default initial seed is used         */
    if (initf==0) init_genrand(5489UL);

    left = N;
    next = state;
    
    for (j=N-M+1; --j; p++) 
        *p = p[M] ^ TWIST(p[0], p[1]);

    for (j=M; --j; p++) 
        *p = p[M-N] ^ TWIST(p[0], p[1]);

    *p = p[M-N] ^ TWIST(p[0], state[0]);
}
//--------------------------------------------------------
/* generates a random number on [0,0xffffffff]-interval */
unsigned long MersenneTwister::genrand_int32(void)
{
    unsigned long y;

    if (--left == 0) next_state();
    y = *next++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}
/*********************************************************************************
*
*			Name:		MRand_Init
*
*			Summary:	Initialize the math library.  Primarily, the random
*							number generator.
*
*			Arguments:
*				LbFourByte		*randSeed	- Seed for generator
*
*			Function return: True unless an error occurs.
*
*********************************************************************************/
bool MersenneTwister::MRand_Init(LbFourByte randSeed)
{
	if(MTRand_IsInited)			  /*we only want to call this once*/
	  return MTRand_IsInited;
	
	/* If random seed is not positive, set it from the system clock */
	if (randSeed <= 0) 
	{
		/*randSeed = (LbFourByte) time(NULL);*/ /*bfe 5.17.10 - this is only unique once a second*/
		
		/*
		 //the seed will repeat every 24 days or so.

		 // If you use 100 (rather than 1000) the seed repeats every 248 days.

		 // Do not make the MISTAKE of using just the tv_usec
		 // This will mean your seed repeats every second.	
		*/
		struct timeval tv;
     	gettimeofday(&tv,NULL);
		randSeed = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
		
		if (randSeed < 0)
		{	randSeed *= -1;
		}
	}
	//mh->random_seed = randSeed;	/* bfe - store it out if were going to*/
	
	/* Initialize the RNG with the seed */
	init_genrand((unsigned long)randSeed);
	
    printf("Mersenne-twister> Initializing random generator with: %d\n", randSeed);
	
	MTRand_IsInited =  true;
	
	return MTRand_IsInited;
}

/*********************************************************************************
*
*			Name:		MRand_GetRandomNumber
*
*			Summary:	This is the ASIM Mersenne twister random number generator.
*
*			Arguments:	None.
*				
*			Function return: A random number between 0.0 and 1.0 (exclusive) = (0,1).
*
*********************************************************************************/
double MersenneTwister::MRand_GetRandomNumber()
{
	double 		randReal;	/* The floating point random number created */
	
	if(!MTRand_IsInited)	/*make sure we have been inited*/
	{
	  	printf(" ASIM !!! error: Mersenne twister MRand_GetRandomNumber called, but not initialized!\n");
		exit(1);
	}
	
	/* Obtain the next random number */

	/* Mersenne Twister RNG */
	LbUsFourByte	u;			/* An unsigned random integer */
	
	do 
	{
		u = (LbUsFourByte) genrand_int32();
	} while (u == 0);
	
	randReal = ((double)u) * (1.0/4294967296.0);  /* divide by 2^32 */
	
	return ((double) randReal);
}

