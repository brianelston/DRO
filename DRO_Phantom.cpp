/*
 *  DRO_Phantom.cpp
 *  DRO_phantom
 *
 *  Created by Brian Elston on 6/22/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *  The phantom is where the pixel data is generated or alternatively stored from an exterior source
 */

#include "DRO_Phantom.h"

#define _USE_MATH_DEFINES	//for PI compatibility

//--------------------------------------------------------
DRO_Phantom::DRO_Phantom()
{
	//pointers
	x_center_vals = NULL;
	y_center_vals  = NULL;
	z_center_vals = NULL;
	DRO_object = NULL;
	x_fine = NULL;
	y_fine = NULL;
	z_fine = NULL;
	DRO_object_int16 = NULL;
	DRO_object_Uint16 = NULL;
	max_float_per_slice = NULL;

	//emission defaults
	body_activity = 1.0;
	center_rod_activity = 0.0;		
	sphere_activity = 4.0;
	hot_voxel_activity = 4.11;
	background_activity=0;	
	negative_voxel_activity = -0.11;
	
	//transmission defaults
	shell_attenuation = 100.0;
	body_attenuation = 0.0;
	center_rod_attenuation = -1000.0;
	sphere_attenuation = 50.0;
	
	num_fine = 10;				//how many divisions for fine sampleing.
	max_float = -1000000.0;
	
	strcpy(OutputFileName, "DRO_phantom_Image");	//for raw data output - verification only
	
	num_x_pixels = 256;
	num_y_pixels = 256;
	image_size_x = 500.0;
	image_size_y = 500.0;
	image_size_half_z = 110.0;	//220.0mm - full size;
	
	dx = image_size_x/num_x_pixels;
	dy = image_size_y/num_y_pixels;
	dz = 2.0; //(220/110) - image_size_z/num_slices
	
	b_show_test_pattern = false;
	b_ShowHotVoxel = false;	//true	bfe 1.4.12 - dont show as default to prevent on encapsulation mode
	test_pattern_slice = -1;
	test_pattern_volume_centerx = -1;
	test_pattern_volume_centery = -1;
	test_pattern_extentxy = 0;
	test_pattern_volume_extentz = 0;
	test_pattern_area_centerx = -1;
	test_pattern_area_centery = -1;
	
	bGaussian_kernel_inited = false;
	x_kernel_vals = NULL;
	y_kernel_vals = NULL;
	g_kernel = NULL;
	g_kernel_new = NULL;
	
	b_CalculateNoise = false;
	AmountNoise = 0.0;	
	b_GaussianSmooth = true;
	smooth_fwhm = 5.0;
	//noisy_slice = -1;
	
	Rescale_Slope_Per_Slice = false;
	b_Create_DICOMDIR = false;
	
	max_sphere5 = 0.0;
	min_sphere5 = 10000;
	b_GaussianBlob_sphere5 = false;
}
//--------------------------------------------------------
DRO_Phantom::~DRO_Phantom()
{
	FreeObject(x_center_vals);
	FreeObject(y_center_vals);
	FreeObject(z_center_vals);
	FreeObject(max_float_per_slice);
	
	FreeObject(DRO_object);
	
	FreeObject(x_fine);
	FreeObject(y_fine);
	FreeObject(z_fine);
	
	FreeObject(DRO_object_int16);
	FreeObject(DRO_object_Uint16);
}
//--------------------------------------------------------
void DRO_Phantom::FreeObject(void * object)
{	
	if(object != NULL)
	{	free(object);
		object = NULL;
	}
}
//--------------------------------------------------------
bool DRO_Phantom::InitAndAllocate(bool braw, int x_dim, int y_dim, int z_dim)
{
	//from the larry pierce - matlab script
	//x_center_vals = (-250+dx/2):dx:(250-dx/2);	//note first value is ~ -249.0
	//y_center_vals = (250-dy/2):-dy:(-250+dy/2);
	//z_center_vals = (-111+dz/2):dz:(111+dz/2);	//this is not correct - close though

	if(braw)
	{ //raw mode loads from user or file
	  num_x_pixels = x_dim;
	  num_y_pixels = y_dim;
	  image_size_half_z = z_dim/2.0;
	}
	
	dx = image_size_x/num_x_pixels;
	dy = image_size_y/num_y_pixels;
	//how many sampling positions
	int half_x = (int) (image_size_x/2);
	int half_y = (int) (image_size_y/2);
	
	if(!braw)
	{
	  num_dx = int(fabs(-half_x+dx/2.0)/dx + fabs(half_x-dx/2.0)/dx) + 1;	//needs +1 to fill out the data set (ie the centermost value)
	  num_dy = int(fabs(half_y-dy/2.0)/dy + fabs(-half_y+dy/2.0)/dy) + 1;
	  num_dz = int(fabs(-image_size_half_z+dz/2.0)/dz + fabs(image_size_half_z-dz/2.0)/dz) + 1;
	}
	else
	{ //raw input mode - take from user input
	  num_dx = x_dim;
	  num_dy = y_dim;
	  num_dz = z_dim;
	}
	num_dro = num_dx * num_dy * num_dz;
	
	//error check final values here agaist num objects
	if(!AreParametersWithinLimits())
	{
		this->~DRO_Phantom();	//dealloc through destructor
		exit(1);
	}
	
	//allocate
	x_center_vals = (double*) malloc(sizeof(double) * num_dx);
	y_center_vals = (double*) malloc(sizeof(double) * num_dy);
	z_center_vals = (double*) malloc(sizeof(double) * num_dz);
	DRO_object = (double*) malloc(sizeof(double) * num_dro);			
	DRO_object_Uint16 = (unsigned short*) malloc(sizeof(short) * num_dro);
	DRO_object_int16 = (short*) malloc(sizeof(short) * num_dro);	
	memset(DRO_object_int16, 0, (sizeof(short) * num_dro));	
	memset(DRO_object_Uint16, 0, (sizeof(short) * num_dro));	
	
	max_float_per_slice = (double*) malloc(sizeof(double) * num_dz);
	memset(max_float_per_slice, 0, (sizeof(double) * num_dz));	
	
	if(x_center_vals == NULL || y_center_vals == NULL || z_center_vals == NULL || DRO_object == NULL || DRO_object_int16 == NULL || DRO_object_Uint16 == NULL)
	{
		printf("Error in allocation (1)! Aborting!\n");
		this->~DRO_Phantom();	//dealloc through destructor
		exit(1);
	}
	
	InitializeDRO_Object();	//Initializes based on EM or ATT
	
	//set the sampling value centers
	for(int i = 0 ; i < num_dx; i++)
	{	x_center_vals[i] = (-half_x + dx/2.0) + (i * dx);
	}
	
	for(int i = 0 ; i < num_dy; i++)
	{	y_center_vals[i] = (half_y - dy/2.0) - (i * dy);
	}
	
	for(int i = 0 ; i < num_dz; i++)
	{	z_center_vals[i] = (-image_size_half_z + dz/2.0) + (i * dz);
	}
	
	x_fine = (double*) malloc(sizeof(double) * num_fine);
	y_fine = (double*) malloc(sizeof(double) * num_fine);
	z_fine = (double*) malloc(sizeof(double) * num_fine);
	
	if(x_fine == NULL || y_fine == NULL || z_fine == NULL)
	{
		printf("Error in allocation (2)! Aborting!\n");
		this->~DRO_Phantom();	//dealloc through destructor
		exit(1);
	}
	memset(x_fine, 0, (sizeof(double) * num_fine));	
	memset(y_fine, 0, (sizeof(double) * num_fine));	
	memset(z_fine, 0, (sizeof(double) * num_fine));	
	
	//set the fine sampling (oversampling) incrementals
	for(int i = 0 ; i < num_fine && num_fine > 1; i++)	//dont do if only 1 subsample
	{	
		x_fine[i] = (-dx/2.0) + (i * dx/(num_fine-1.0));
		y_fine[i] = (dy/2.0) - (i * dy/(num_fine-1.0));
		z_fine[i] = (-dz/2.0) + (i * dz/(num_fine-1.0));
	}
	
	return true;
}
//--------------------------------------------------------
bool DRO_Phantom::CreateGeometry()
{	//create the phantom objects - calls into subsampleing for emissio or transmission
	double pi = M_PI;	//set up 3.14 etc...
	const int num_spheres = 6;
	DRO_Sphere sphere[num_spheres];
	
	//the spheres - note optimization depends on their location
	sphere[0].SetDRO_SphereStats(57.2*cos(1.0*pi/3), 57.2*sin(1.0*pi/3), 10/2.0, 30.0);
	sphere[1].SetDRO_SphereStats(57.2*cos(2.0*pi/3), 57.2*sin(2.0*pi/3), 13/2.0, 30.0);
	sphere[2].SetDRO_SphereStats(57.2*cos(3.0*pi/3), 57.2*sin(3.0*pi/3), 17/2.0, 30.0);
	sphere[3].SetDRO_SphereStats(57.2*cos(4.0*pi/3), 57.2*sin(4.0*pi/3), 22/2.0, 30.0);
	sphere[4].SetDRO_SphereStats(57.2*cos(5.0*pi/3), 57.2*sin(5.0*pi/3), 28/2.0, 30.0);
	sphere[5].SetDRO_SphereStats(57.2*cos(6.0*pi/3), 57.2*sin(6.0*pi/3), 37/2.0, 30.0);
	
	double mean = 0.0;
	
	double *x_fine_centers = NULL;
	double *y_fine_centers = NULL;
	double *z_fine_centers = NULL;
	
	x_fine_centers = (double*) malloc(sizeof(double) * num_fine);
	y_fine_centers = (double*) malloc(sizeof(double) * num_fine);
	z_fine_centers = (double*) malloc(sizeof(double) * num_fine);
	
	if(x_fine_centers == NULL || y_fine_centers == NULL || z_fine_centers == NULL)
	{
		printf("Error in allocation (-CreateGeometry-)! Aborting!\n");
		this->~DRO_Phantom();	//dealloc through destructor
		exit(1);
	}	
	
	const short num_display_count = 10;	//give the user some feedback now and then

	//loop over all sample centers
	for(int this_z_center_index = 0; this_z_center_index < num_dz; this_z_center_index++) //probably want +1
	{
		if(this_z_center_index % num_display_count == 0) 
			printf("slice (%d): z:DRO_phantom::CreateGeometry - progress\n", this_z_center_index);
		
		for(int this_y_center_index = 0; this_y_center_index < num_dy; this_y_center_index++) 
		{
			for(int this_x_center_index = 0; this_x_center_index < num_dx; this_x_center_index++) 
			{
				//double ngrid implementation				
				for(int i = 0; i < num_fine; i++)
				{
					x_fine_centers[i] = x_center_vals[this_x_center_index] + x_fine[i];
					y_fine_centers[i] = y_center_vals[this_y_center_index] + y_fine[i];
					z_fine_centers[i] = z_center_vals[this_z_center_index] + z_fine[i];
				}
				
				//call into child for geometry & value; emission or transmission
				mean = CreateGeometryForSubsample(sphere, num_spheres, x_fine_centers, y_fine_centers, z_fine_centers);
				
				SetDROPixelValue(this_y_center_index, this_x_center_index, this_z_center_index, mean);
			}
		}
	}	///end z,y,x loop
		
	//bfe 3.23.12 - unfortunately these loops are wasted on CT - find min and max sphere 5
	if(b_GaussianBlob_sphere5)
	{
	  for(int this_z_center_index = 0; this_z_center_index < num_dz; this_z_center_index++) //probably want +1
	  {
		  
		  for(int this_y_center_index = 0; this_y_center_index < num_dy; this_y_center_index++) 
		  {
			  for(int this_x_center_index = 0; this_x_center_index < num_dx; this_x_center_index++) 
			  {
				  FindMinMax_Sphere5(sphere, num_spheres, this_x_center_index, this_y_center_index, this_z_center_index);
			  }
		  }
	  }
	  
	  //in PET we need to adjust the values for sphere 5
	  for(int this_z_center_index = 0; this_z_center_index < num_dz; this_z_center_index++) //probably want +1
	  {
		  
		  for(int this_y_center_index = 0; this_y_center_index < num_dy; this_y_center_index++) 
		  {
			  for(int this_x_center_index = 0; this_x_center_index < num_dx; this_x_center_index++) 
			  {
				  //call into child for geometry & value; emission or transmission
				  bool bchanged = false;
				  mean = Resample_Sphere5(sphere, num_spheres, x_fine_centers, y_fine_centers, z_fine_centers, this_x_center_index, this_y_center_index, this_z_center_index, bchanged);
				  //fine centers not used
				  if(bchanged)
				  {	SetDROPixelValue(this_y_center_index, this_x_center_index, this_z_center_index, mean);
				  }
			  }
		  }
	  }	///end z,y,x loop
	}
	
	FreeObject(x_fine_centers);
	FreeObject(y_fine_centers);
	FreeObject(z_fine_centers);
	
	return true;
}
//--------------------------------------------------------
void DRO_Phantom::SetDROPixelValue(int y, int x, int z, double activity)
{	//modularize in case the orientation of data changes
	
	if(num_dz < z)
	{	printf("SetDROPixelValue: warining - too few slices to set pixel at slice %d.\n", z);
		return;
	}
	
	DRO_object[x + (y * num_dx) + (z * num_dx * num_dy)] = activity;
}//--------------------------------------------------------
void DRO_Phantom::SetDROIntValue_SHORT(int y, int x, int z, double activity)
{	//modularize in case the orientation of data changes
	
	if(num_dz < z)
	{	printf("SetDROIntValue_SHORT: warining - too few slices to set pixel at slice %d.\n", z);
		return;
	}
	
	double d_activity = 0;
	if(Rescale_Slope_Per_Slice)
	{
		d_activity = activity/max_float_per_slice[z];
	}
	else
	{
		d_activity = activity/max_float;
	}
	
	short s16_activity;

	if(d_activity >= 0)
	{	s16_activity = (short)(d_activity * MAX_IMAGE_VALUE);
		
	}
	else	//< =0...allot for slight change in scale length
	{	s16_activity = (short)(- d_activity * MIN_IMAGE_VALUE);
	}
	
	DRO_object_int16[x + (y * num_dx) + (z * num_dx * num_dy)] = s16_activity;
}
//--------------------------------------------------------
double DRO_Phantom::GetMean (double * array, double N)
{	// function calculating mean
	double sum = 0.0;
	for (int i = 0; i < N; i++)
	{	sum = sum + array [i];
	}
	return sum/N;
} 
//--------------------------------------------------------
void DRO_Phantom::CopyData(DRO_Phantom &newph, const DRO_Phantom &oldph)
{	//we need a copy for the command line input parameters transfer to child instantiantions
	//NOTE: New items added to command parameters file will need to also be added here! (since we store in a temp file then copy out - during the input process)

    // Only do assignment if oldph is a different object.
    if (&newph != &oldph) 
	{
		newph.num_fine = oldph.num_fine;
		newph.body_activity = oldph.body_activity;
		newph.center_rod_activity = oldph.center_rod_activity;
		newph.sphere_activity = oldph.sphere_activity;
		newph.hot_voxel_activity = oldph.hot_voxel_activity;
		newph.negative_voxel_activity = oldph.negative_voxel_activity;
		newph.background_activity = oldph.background_activity;
		newph.shell_attenuation = oldph.shell_attenuation;
		newph.body_attenuation = oldph.body_attenuation;
		newph.center_rod_attenuation = oldph.center_rod_attenuation;
		newph.sphere_attenuation = oldph.sphere_attenuation;
		newph.image_size_x = oldph.image_size_x;
		newph.image_size_y = oldph.image_size_y;
		newph.image_size_half_z = oldph.image_size_half_z;
		newph.num_x_pixels = oldph.num_x_pixels;
		newph.num_y_pixels = oldph.num_y_pixels;
		newph.dz = oldph.dz;
		newph.b_show_test_pattern = oldph.b_show_test_pattern;
		newph.b_ShowHotVoxel = oldph.b_ShowHotVoxel;
		
		newph.test_pattern_slice = oldph.test_pattern_slice;
		newph.test_pattern_volume_centerx = oldph.test_pattern_volume_centerx;
		newph.test_pattern_volume_centery = oldph.test_pattern_volume_centery;
		newph.test_pattern_extentxy = oldph.test_pattern_extentxy;
		newph.test_pattern_volume_extentz = oldph.test_pattern_volume_extentz;
		newph.test_pattern_area_centerx = oldph.test_pattern_area_centerx;
		newph.test_pattern_area_centery = oldph.test_pattern_area_centery;
		
		newph.b_CalculateNoise = oldph.b_CalculateNoise;
		newph.AmountNoise = oldph.AmountNoise;
		newph.b_GaussianSmooth = oldph.b_GaussianSmooth;
		newph.smooth_fwhm = oldph.smooth_fwhm;
		//newph.noisy_slice = oldph.noisy_slice;
		newph.Rescale_Slope_Per_Slice = oldph.Rescale_Slope_Per_Slice;
		newph.b_Create_DICOMDIR = oldph.b_Create_DICOMDIR;
		newph.b_GaussianBlob_sphere5 = oldph.b_GaussianBlob_sphere5;
    }
}
//--------------------------------------------------------
bool DRO_Phantom::AreInputValuesValid()
{	//input testing generic to both emission and transmission
	const int max_image_sizexy = 20000;
	const int max_image_sizez = 5000;
	
	const int max_pixels_xy = 5096;
	const int max_slicethickness_z = 100;
	
	if(num_fine < 1 || num_fine > MAX_NUM_FINE)
	{
		printf(" AreInputValuesValid> !!! Error: Number of fine division sampleing (num_fine) must be between 1 and %d\n", MAX_NUM_FINE);
		return false;
	}
	
	//image size
	if(image_size_x < 1 || image_size_x > max_image_sizexy)
	{
		printf(" AreInputValuesValid> !!! Error: Image size x (image_size_x) must be between 1 and %d\n", max_image_sizexy);
		return false;
	}
	if(image_size_y < 1 || image_size_y > max_image_sizexy)
	{
		printf(" AreInputValuesValid> !!! Error: Image size y (image_size_y) must be between 1 and %d\n", max_image_sizexy);
		return false;
	}
	if(image_size_half_z < 1 || image_size_half_z > max_image_sizez)
	{
		printf(" AreInputValuesValid> !!! Error: Image size z (image_size_z) must be between 1 and %f\n", (image_size_half_z * 2));	
		return false;
	}
	
	//number of pixels
	if(num_x_pixels < 1 || num_x_pixels > max_pixels_xy)
	{
		printf(" AreInputValuesValid> !!! Error: Number pixels x (num_x_pixels) must be between 1 and %d\n", max_pixels_xy);
		return false;
	}
	if(num_y_pixels < 1 || num_y_pixels > max_pixels_xy)
	{
		printf(" AreInputValuesValid> !!! Error: Number pixels y (num_y_pixels) must be between 1 and %d\n", max_pixels_xy);
		return false;
	}
	if(dz < 0.000000001 || dz > max_slicethickness_z)
	{
		printf(" AreInputValuesValid> !!! Error: Slice thickness z (slice_thickness_z) must be larger than 0 and less than %d\n", max_slicethickness_z);
		return false;
	}
	
	return true;
}





