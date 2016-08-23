/*
 *  DRO_Volume.cpp
 *  DRO_phantom
 *
 *  Created by Brian Elston on 6/24/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#include "DRO_Sphere.h"

DRO_Sphere::DRO_Sphere()
{
	 c[0] = 0.0;
	 c[1] = 0.0;
	 r = 0.0;				//radius
	 z_extent = 30.0;		//slice central to object, in -mm
}
//--------------------------------------------------------
void DRO_Sphere::SetDRO_SphereStats(double c1, double c2, double rin, double zin)
{
	 c[0] = c1;		//centerpoint
	 c[1] = c2;
	 r = rin;		//radius
	 z_extent = zin;
}
//--------------------------------------------------------
double DRO_Sphere::GetDistanceValue(double x, double y, double z)
{
	return sqrt(pow((x-c[0]),2) + pow((y-c[1]), 2) + pow((z + z_extent), 2));
}
