/*
 *  DRO_DICOM_Header_CT.h
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/18/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#include "DRO_DICOM_Header.h"

#ifndef DRO_DICOM_Header_CT_H
#define DRO_DICOM_Header_CT_H

//derived class for transmission header

class DRO_DICOM_Header_CT : public DRO_DICOM_Header
{
public:

	void SetDICOMHeaderFields(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj, int slice, char * uid_fname, char * InFile_UID_Date_fname, DcmFileFormat * dcmfile, bool braw_data);		//set in-code DICOM fields
	
private:

	//virtual overrides
	void SetPixelSpacingDcmInfo(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj);
	void SetHeaderSliceInfo(DcmDataset *dataset, int slice, DRO_Phantom * DRO_phantom_obj);
	
	void SetInstitutionUIDDcmInfo(DcmDataset *dataset, int slice, char * uid_fname, char * InFile_UID_Date_fname, DcmFileFormat * dcmfile, char * UID_prefix);						//UID - sub-range prefix from file, with DRO prefix, and slice
};

#endif