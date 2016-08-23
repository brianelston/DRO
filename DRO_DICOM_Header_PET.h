/*
 *  DRO_DICOM_Header_PET.h
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/18/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#include "DRO_DICOM_Header.h"


#ifndef DRO_DICOM_Header_PET_H
#define DRO_DICOM_Header_PET_H

//derived class for emission header

class DRO_DICOM_Header_PET : public DRO_DICOM_Header
{
public:

	void SetDICOMHeaderFields(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj, int slice, char * uid_fname, char * InFile_UID_Date_fname, DcmFileFormat * dcmfile, bool braw_data);		//set in-code DICOM fields
	
private:

	//virtual overrides
	void SetPixelSpacingDcmInfo(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj);
	void SetHeaderSliceInfo(DcmDataset *dataset, int slice, DRO_Phantom * DRO_phantom_obj);
	
	//void SetUID_Fields(char * sub_range_prefix, char * datecode);
	void SetInstitutionUIDDcmInfo(DcmDataset *dataset, int slice, char * uid_fname, char * InFile_UID_Date_fname, DcmFileFormat * dcmfile, char *UID_prefix);						//UID - sub-range prefix from file, with DRO prefix, and slice
	
	//specific to emission
	void SetRescaleSlopeDcmInfo(DcmDataset *dataset, int slice, DRO_Phantom * DRO_phantom_obj);
	
	//for raw data set to some set value
	void SetRescaleSlopeDcmInfo_toHARDVALUE(DcmDataset *dataset, int slice, float value);
	
};

#endif