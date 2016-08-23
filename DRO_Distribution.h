/*
 *  DRO_Distribution.h
 *  DRO_phantom
 *
 *  Created by Brian Elston on 8/8/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 
   Adds Poisson noise for a given mean.
  Based on the poisson routine of Michel Defrise
  20 Jan 97: Paul Kinahan 
  
    Adds Gaussian noise for a given mean and stddev.
  Based on the routine gasdev() from NR, 2nd Ed, pg 289
  17 Jan 97: Paul Kinahan 
  
    History:
    04-MAR-97, CC-UPMC : The random value ranval should be
    strictly less than 1.0, otherwise we end up with an
    infinite loop !

    22-AUG-00, CC-SHFJ : LONG_MAX can vary from machine to
                         machine. Therefore, I prefer to
                         use RAND_MAX (that goes with rand()),
                         even if random() is better than rand()
  
	15-JULY-11	BFE		: Adapted for use in DRO noise routine from ASIM code.
	
 *	Contributors: R. Harrison, P. Kinahan, T. Lewellen
 */
 
#ifndef DRO_Distribution_h
#define DRO_Distribution_h

#include "Constants.h"
#include "Mersenne.h"

#define THRESHOLD 25 /* where to switch from Poisson to Normal */

class DRO_Distribution
{
public:
	
	int poisson(float mean);
	
	float gaussian(float mean, float sdev);
	
private:

	double UniformVal;					//we store the value we get
	
	double UniformDistribution();		//return a value 0-1.0
	
	void Init_Srand(long r_seed);		//init the random generator
	
	//float gaussian(float mean, float sdev);
#ifdef MERSENNE_RAND
	MersenneTwister	Mersenne_obj;
	//for now, we operate on the mersenne twister as an object, while the C random() is performed through static calls
#endif
};

#endif