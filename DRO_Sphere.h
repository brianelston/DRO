/*
 *  DRO_Volume.h
 *  DRO_phantom
 *
 *  Created by Brian Elston on 6/24/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */
 
#ifndef DRO_Sphere_h
#define DRO_Sphere_h

#include <math.h>

class DRO_Sphere
{	//class for defining a sphere
public:

	DRO_Sphere();
	
	void SetDRO_SphereStats(double c1, double c2, double r, double z = 30.0);
	double GetDistanceValue(double x, double y, double z);

	inline double Get_r()	{ return r;};
	
private:
	
	//for spheres
	double r;		//radius
	double c[2];			//x/y centerpoint
	double z_extent;		//max z

};

#endif