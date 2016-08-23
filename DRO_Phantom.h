/*
 *  DRO_Phantom.h
 *  DRO_phantom
 *
 *  Created by Brian Elston on 6/22/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */
#ifndef DRO_Phantom_H
#define DRO_Phantom_H

//the phantom object

#include "Constants.h"
#include "DRO_Sphere.h"

class DRO_Phantom
{	//the pixel object geometry - for emission or transmission
public:
	
	DRO_Phantom();
	~DRO_Phantom();
	
	bool InitAndAllocate(bool braw = false, int x_dim = 1, int y_dim = 1, int z_dim = 1);															///memory allocation and initialization
	bool CreateGeometry();															//calculate pixel values
	
	virtual bool SetDICOMPixelData(DcmDataset *dataset, int slice) = 0;				//set the dicom file to have pixel data
	virtual bool ConvertImageDataToOutputFormat() = 0;								//convert to output format, including noise if applicable
	virtual	bool AreInputValuesValid() = 0;											//error check input values
	virtual bool WriteDROfile() = 0;												//write entire data file - as a data block - testing only
	
	void CopyData(DRO_Phantom &newph, const DRO_Phantom &oldph);					//copy function, since when we read input we store in a temp object, then transfer once we have a type od sim
	
	//setters and getters - well leave as public since setters defy the idea of protected data
	//emission avtivity
	double 	body_activity; 
	double 	center_rod_activity;
	double 	sphere_activity;
	double 	hot_voxel_activity;
	double 	background_activity; 
	double 	negative_voxel_activity;
	bool	b_ShowHotVoxel;			  //do we put the hot (& cold) voxel in the image (for emission)?
	
	//transmission mu
	double 	shell_attenuation;
	double 	body_attenuation;
	double 	center_rod_attenuation;
	double 	sphere_attenuation;
	
	//image size/pixels
	double image_size_x;
	double image_size_y;
	double image_size_half_z;
	int num_x_pixels;
	int num_y_pixels;
	
	int		num_fine;					//how many subdivisions for sampleing - ie oversampling
	
	double dz;							//distance between slices, in mm
	
	bool b_show_test_pattern;			//do we add the test? valid for EM only
	
	int test_pattern_slice;				//we apply 2  test patterns defined by these 6 parameters
	double test_pattern_area_centerx;	//first pattern is a single slice area
	double test_pattern_area_centery;
	double test_pattern_extentxy;		//both patterns have same x/y extent
	double test_pattern_volume_centerx;	//second pattern is a multiple slice volume
	double test_pattern_volume_centery;
	double test_pattern_volume_extentz;	//volume patterns total z extent
	
	bool b_CalculateNoise;				//do we add noise? valid for EM only
	double AmountNoise;					//amount noise. Note all phantom vars must be in parent class, though EM only
	bool b_GaussianSmooth;				//to do smoothing or not?
	double smooth_fwhm;					//FWHM of gaussian smoothing to apply
	//int noisy_slice;					//we apply noise (& smoothing) to a single slice
	
	bool Rescale_Slope_Per_Slice;		//GE want to have a rescale slope per slice; i.e. - based on max pixel value in a given slice, not the image
	bool b_Create_DICOMDIR;				//Do we create a DICOMDIR, and if we do, we remove the .dcm filename extension (since it is incompatible)
	
	//getters for vars only set internally
	inline int Get_num_dx()	{ return num_dx;};
	inline int Get_num_dy()	{ return num_dy;};
	inline int Get_num_dz()	{ return num_dz;};
	inline int Get_num_dro()	{ return num_dro;};
	
	inline double Get_dx()	{ return dx;};
	inline double Get_dy()	{ return dy;};
	inline double Get_dz()	{ return dz;};
	inline double Get_max_float()	{ return max_float;};
	inline void Set_max_float(double inval)	{ max_float = inval;};
	
	inline double* Get_x_center_vals()	{ return x_center_vals;};
	inline double* Get_y_center_vals()	{ return y_center_vals;};
	inline double* Get_z_center_vals()	{ return z_center_vals;};
	
	//pre-existing file modification
	virtual bool GetDICOMPixelData(DcmDataset *dataset, int slice) = 0;		//testing only - replace dicom head, using pre-existing pixel data
	virtual void Calculate_Max_Float() = 0;
	virtual double Calculate_Max_Float_For_Slice(int slice) = 0;
	
	double *max_float_per_slice;
	double max_sphere5;
	double min_sphere5;
	
	bool b_GaussianBlob_sphere5;				//do we do regular sphere or gaussian blob?

	//bfe 12.31.12 - should be protected - but move outside to allow raw pixel data input directly into data structure
	double * 			DRO_object;				//the object in 64-bit doubles: create geometry with best precision possible
	
protected:

	unsigned short * 	DRO_object_Uint16;		//2 byte, 16 bit, greyscale - final output format transmission
	short * 			DRO_object_int16;		//2 byte, 16 bit, greyscale - final output format emission
	
	//the number of samples in a given orientation
	int num_dx;		
	int num_dy;
	int num_dz;
	int num_dro;		//total number of data values
	
	double max_float;	//max value in image: 0..1; store it, its used in the header
	
	char OutputFileName[256];											//file output name for raw data
	
	//gaussian noise vars
	bool 	bGaussian_kernel_inited;
	double g_sd;					//standard deviation of FWHM
	double g_sd_2x;
	int num_x_kernel;				//x,y kernel sizes
	int num_y_kernel;
	double * x_kernel_vals;			//x,y serial kernel
	double * y_kernel_vals;
	double * g_kernel;				//the computed kernel
	double * g_kernel_new;			//the normalized kernel for that pixel
	
	double GetMean (double * array, double N);							//average of fine sampleing
	
	void SetDROPixelValue (int x, int y, int z, double activity);		//set an individual pixel value in double precision object
	void SetDROIntValue_SHORT(int y, int x, int z, double activity);
	
	void FreeObject(void * object);										//dealloc helper
	
	virtual bool AreParametersWithinLimits() = 0;						//checks run-time params against calculated limits
		//the sampleing center points
	double *x_center_vals;
	double *y_center_vals;
	double *z_center_vals;

private:

	virtual void InitializeDRO_Object() = 0;							//init to specific value
	
	//create the actual geometry for that type of pixel object (emission or transmission)
	virtual double CreateGeometryForSubsample(DRO_Sphere sphere[], int num_spheres, double x_fine_centers [], double y_fine_centers [], double z_fine_centers []) = 0;
	virtual double Resample_Sphere5(DRO_Sphere sphere[], int num_spheres, double x_fine_centers [], double y_fine_centers [], double z_fine_centers [], int this_x_center_index, int this_y_center_index, int this_z_center_index, bool & bchanged) = 0;
	virtual 	void FindMinMax_Sphere5(DRO_Sphere sphere[], int num_spheres, int this_x_center_index, int this_y_center_index, int this_z_center_index) = 0;
	
	double dx;			//sampleing distance in x orgin
	double dy;			//sampleing distance in y orgin
	
	//the incremental sampleing amounts
	double *x_fine;
	double *y_fine;
	double *z_fine;
};
#endif