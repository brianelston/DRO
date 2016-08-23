/*
 *  DRO_phantom_CT.h
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/19/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#include "DRO_Phantom.h"

#ifndef DRO_phantom_CT_H
#define DRO_phantom_CT_H

//TRANSMISSION - CT

class DRO_Phantom_CT : public DRO_Phantom
{
public:

	DRO_Phantom_CT();
	
	bool AreInputValuesValid();				//check validity
	
	bool WriteDROfile();					//write data block output - test & verification only
	
	bool SetDICOMPixelData(DcmDataset *dataset, int slice);			//DICOM pixel data access
	
	bool GetDICOMPixelData(DcmDataset *dataset, int slice);			//for testing only
	
	void Calculate_Max_Float();
	double Calculate_Max_Float_For_Slice(int slice);

protected:
	
	void InitializeDRO_Object();

	double CreateGeometryForSubsample(DRO_Sphere sphere[], int num_spheres, double x_fine_centers [], double y_fine_centers [], double z_fine_centers []);	//geometery
	
	double Resample_Sphere5(DRO_Sphere sphere[], int num_spheres, double x_fine_centers [], double y_fine_centers [], double z_fine_centers [], int this_x_center_index, int this_y_center_index, int this_z_center_index, bool & bchanged);
	void FindMinMax_Sphere5(DRO_Sphere sphere[], int num_spheres, int this_x_center_index, int this_y_center_index, int this_z_center_index);
	
	bool ConvertImageDataToOutputFormat();			//load output buffer - 2 byte unsigned short
	
private:

	bool AreParametersWithinLimits();				//checks run-time params against calculated limits
	
	bool CovertImageto_12bit_int16();				//correct format for transmission output-  short
};

#endif