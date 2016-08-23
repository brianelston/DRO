/*
 *  Mersenne.h
 *  DRO
 *
 *  Copyright 2011 UWMC Radiology IRL - University of Washington Nuclear Medicine. All rights reserved.
 *	Author: Brian Elston - bfe, , S. Gillespie
 *	Contributors: R. Harrison, P. Kinahan, T. Lewellen
 
 1.20.9		bfe		Initial pass.
 8.8.11		bfe		Integration into DRO.
 *
 */

#ifndef __MERSENNE_H__
#define __MERSENNE_H__

/*this may not support other machines outside of the MAC; 1.20.2009*/
typedef	long			LbFourByte;
typedef	unsigned long	LbUsFourByte;

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UMASK 0x80000000UL		/* most significant w-r bits */
#define LMASK 0x7fffffffUL		/* least significant r bits */
#define MIXBITS(u,v) ( ((u) & UMASK) | ((v) & LMASK) )
#define TWIST(u,v) ((MIXBITS(u,v) >> 1) ^ ((v)&1UL ? MATRIX_A : 0UL))

class MersenneTwister
{

public:

	//MersenneTwister::MersenneTwister();
	MersenneTwister();

	/*MERSENNE TWISTER HOOKS*/
	bool		MRand_Init(LbFourByte randSeed);

	double 			MRand_GetRandomNumber(void);

private:

	unsigned long state[N]; /* the array for the state vector  */
	int left;
	int initf;
	unsigned long *next;
	bool		MTRand_IsInited;

	/*SUPPORT FUNCTIONS - Do not call these directly from simulation code*/
	/* initializes state[N] with a seed */
	void init_genrand(unsigned long s);

	void next_state(void);

	/* generates a random number on [0,0xffffffff]-interval */
	unsigned long genrand_int32(void);
};

#endif