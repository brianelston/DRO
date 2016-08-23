/*
 *  DRO_Distribution.c
 *  DRO_phantom
 *
 *  Created by Brian Elston on 8/8/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#include "DRO_Distribution.h"

//--------------------------------------------------------
double DRO_Distribution::UniformDistribution()
{
#ifndef MERSENNE_RAND

   UniformVal = ((double)random() / (double)(RAND_MAX));	
  
#else
  /*USE MERSENNE TWISTER*/
  UniformVal = (double) Mersenne_obj.MRand_GetRandomNumber();
  
#endif

  return (UniformVal);
}
//--------------------------------------------------------
void DRO_Distribution::Init_Srand(long r_seed)
{
  long the_seed;
  
  if(r_seed < 0)
  {
	the_seed = time(NULL);
  }
  else
  {
  	the_seed = r_seed;  
  }
  
  srandom(the_seed);	//init the randomizer
}
//--------------------------------------------------------
int DRO_Distribution::poisson(float mean)
{
  int count;
  double prob, sum, ranval;
  static bool init_randomgenerator = false;

  if (mean > THRESHOLD) 
  {		return (int) (gaussian(mean, sqrt(mean)) + 0.5);
  }

  if(init_randomgenerator == false) 
  {

	#ifndef MERSENNE_RAND
		Init_Srand(-1);
	   /* printf(" poisson> RAND_MAX = %0ld\n",RAND_MAX); */
	#else
		Mersenne_obj.MRand_Init(-1);
	#endif

    init_randomgenerator = true;
  }

  /* use gamma distribution for expected time to get 'count' events */

  count = 0;
  prob = exp(-mean);
  sum = prob;

  /*
     ranval should be strictly less than 1.0, unless we will never get in a finite
     time Sum(count)=ranval.

     prob(count) = probability of getting 'count'
     sum(count)  = probability of getting a number equal or below 'count'
  */
  ranval = 1.0;
  while(ranval == 1.0) 	/*bfe- I think this should be >= 1.0, but its legacy*/
  {	
	  ranval = UniformDistribution();
  }
  
  while ((ranval > sum) && (count < 1000)) 
  {
    count++;
    prob *= (double)mean / (double)count; /* gamma distribution */
    sum += prob;
  }
  
  if (count == 1000) 
  {
    printf(" poisson> !!! ACHTUNG infinite loop, will return the (int) mean value\n");
    count = (int) mean;
  }
  return count; 
}    
//--------------------------------------------------------
float DRO_Distribution::gaussian(float mean, float stddev)
{
  static int iset = 0;
  static float gset;
  float fac, rsq, v1, v2;
  static bool init_randomgenerator = false;

  if(init_randomgenerator == false) 
  {
  
	#ifndef MERSENNE_RAND
		Init_Srand(-1);
	#else
		Mersenne_obj.MRand_Init(-1);
	#endif

    init_randomgenerator = true;
  }
  
  if (iset == 0) 
  {
    do 
	{
        v1 = 2.0 * UniformDistribution() - 1.0;
        v2 = 2.0 * UniformDistribution() - 1.0;
        rsq = v1 * v1 + v2 * v2;
    } while (rsq >= 1.0 || rsq == 0.0);
	
    fac = sqrt(-2.0 * log(rsq) / rsq);
    gset = v1 * fac;
    iset = 1;
    return (float) stddev * v2 * fac + mean;
  } 
  else 
  {
    iset = 0;
    return stddev * gset + mean;
  }
}  