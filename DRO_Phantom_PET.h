/*
 *  DRO_Phantom_PET.h
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/19/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#include "DRO_Phantom.h"
#include "DRO_Distribution.h"

#ifndef DRO_Phantom_PET_H
#define DRO_Phantom_PET_H

//EMISSION - PET

class DRO_Phantom_PET : public DRO_Phantom
{
public:

	DRO_Phantom_PET();
	
	bool AreInputValuesValid();					//checks input params

	bool WriteDROfile();						//write data block to output - no header: for test purposes
	
	bool SetDICOMPixelData(DcmDataset *dataset, int slice);		//set the pixel data to DICOM pixel location
	
	//----------Special cases for loading pixel data from pre-existing DICOM stack-------------not normal operation - testing
	bool GetDICOMPixelData(DcmDataset *dataset, int slice);		//get he pixel data
	
	void Calculate_Max_Float();					//for special case when we pre-load pixel data
	double Calculate_Max_Float_For_Slice(int slice);
	
protected:
	
	void InitializeDRO_Object();								//init the double object
	
	double CreateGeometryForSubsample(DRO_Sphere sphere[], int num_spheres, double x_fine_centers [], double y_fine_centers [], double z_fine_centers []);	//calculates the subsample geometry
	
	bool ConvertImageDataToOutputFormat();						//adds hot/cold pixel & preps format for output - stores in signed int
	
	double Resample_Sphere5(DRO_Sphere sphere[], int num_spheres, double x_fine_centers [], double y_fine_centers [], double z_fine_centers [], int this_x_center_index, int this_y_center_index, int this_z_center_index, bool & bchanged);
	void FindMinMax_Sphere5(DRO_Sphere sphere[], int num_spheres, int this_x_center_index, int this_y_center_index, int this_z_center_index);

	
private:

	void SetHotPixelValue();					//set hot pixel value
	void SetNegativePixelValue();				//set cold pixel value
	
	bool ScaleImage();							//scale pixels to 0 - 1.0
	bool CovertImageto_int16();					//correct format for emission output-  short

	bool AreParametersWithinLimits();			//checks run-time params against calculated limits
	
	void AddCheckerBoardPatterns();				//add checkerboard test paterns
	void AddCheckerBoard(bool bvolume);			//adds the volume or area test pattern
	
	//random number generation - noise and smoothing
	DRO_Distribution 	Distribution_obj;	
	bool AddNoiseAndSmooth();	
	void AddNoise(float calibration, short* in, int startx, int endx, int starty, int endy);	//calibration strictly 0-1.0 percentile, and is scaled to pixel variation allowed based on MAX_IMAGE_VALUE
	void GaussianSmooth2D(double g_fwhm, short* in, short* out, int slice, int startx, int endx, int starty, int endy);
	void GaussianSmooth2D_Allocate(double g_fwhm);
	
	void AddNoise_double(float calibration, double* in);
	
};

#endif