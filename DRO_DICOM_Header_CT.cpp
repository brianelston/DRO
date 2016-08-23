/*
 *  DRO_DICOM_Header_CT.cpp
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/18/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#include "DRO_DICOM_Header_CT.h"

void DRO_DICOM_Header_CT::SetDICOMHeaderFields(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj, int slice, char * uid_fname, char * InFile_UID_Date_fname, DcmFileFormat * dcmfile, bool braw_data)
{
	//Fill in header fields........................
	SetImageDataDcmInfo(dataset, DRO_phantom_obj);
	
	SetPixelSpacingDcmInfo(dataset, DRO_phantom_obj);
	SetSliceThicknessDcmInfo(dataset, DRO_phantom_obj);
	
	SetHeaderSliceInfo(dataset, slice, DRO_phantom_obj);
	
	//set instance ID
	if(!braw_data)
	{ //DRO slice data
	  SetInstitutionUIDDcmInfo(dataset, slice, uid_fname, InFile_UID_Date_fname, dcmfile, UID_DRO_PREFIX);
	}
	else
	{ //bfe 1.2.13 - raw data encapsulation
	  SetInstitutionUIDDcmInfo(dataset, slice, uid_fname, InFile_UID_Date_fname, dcmfile, UID_DRES_PREFIX);
	}
}
//--------------------------------------------------------
void DRO_DICOM_Header_CT::SetPixelSpacingDcmInfo(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj)
{
	//number of slices and whatnot
	char temp[256] = {'\0'};
	char val1[256] = {'\0'};
	char val2[256] = {'\0'};
	
	if(!dataset->tagExistsWithValue(DCM_ReconstructionDiameter))
	{
		printf("ERROR (SetPixelSpacingDcmInfo): Cannot retrieve required tag from header file: DCM_ReconstructionDiameter!\n");
		return;
	}
	
	sprintf(val1, "%f", atof(FindAndGetItemOFString(dataset, DCM_ReconstructionDiameter).c_str())/DRO_phantom_obj->Get_num_dx());
	sprintf(val2, "%f", atof(FindAndGetItemOFString(dataset, DCM_ReconstructionDiameter).c_str())/DRO_phantom_obj->Get_num_dy());
	RemoveLeadingZeros(val1);
	RemoveLeadingZeros(val2);
	sprintf(temp, "%s\\%s", val1, val2);
	dataset->putAndInsertString(DCM_PixelSpacing, temp);
}
//--------------------------------------------------------
void DRO_DICOM_Header_CT::SetHeaderSliceInfo(DcmDataset *dataset, int slice, DRO_Phantom * DRO_phantom_obj)
{	//slice info
	char temp[256] = {'\0'};
	sprintf(temp, "%d", (slice+1));
	dataset->putAndInsertString(DCM_InstanceNumber, temp);
	
	if(DRO_phantom_obj->Get_z_center_vals()[slice] == (int) DRO_phantom_obj->Get_z_center_vals()[slice])	//then an int
	{	sprintf(temp, "%d", (int) DRO_phantom_obj->Get_z_center_vals()[slice]);
		dataset->putAndInsertString(DCM_SliceLocation, temp);
		
		sprintf(temp, "%.8f\\%.8f\\%d", DRO_phantom_obj->Get_x_center_vals()[0], -DRO_phantom_obj->Get_y_center_vals()[0], (int) DRO_phantom_obj->Get_z_center_vals()[slice]);
		dataset->putAndInsertString(DCM_ImagePositionPatient, temp);
	}
	else	//encode as float
	{	sprintf(temp, "%f", DRO_phantom_obj->Get_z_center_vals()[slice]);
		dataset->putAndInsertString(DCM_SliceLocation, temp);
		
		sprintf(temp, "%.8f\\%.8f\\%f", DRO_phantom_obj->Get_x_center_vals()[0], -DRO_phantom_obj->Get_y_center_vals()[0], DRO_phantom_obj->Get_z_center_vals()[slice]);
		dataset->putAndInsertString(DCM_ImagePositionPatient, temp);
	}
}
//--------------------------------------------------------
void DRO_DICOM_Header_CT::SetInstitutionUIDDcmInfo(DcmDataset *dataset, int slice, char * uid_fname, char * InFile_UID_Date_fname, DcmFileFormat * dcmfile, char * UID_prefix)
{
	//set any unique UID's...
	//bfe 9.22.11 - need to read prefix from a read-only UID file - the UID prefix needs to be assigned
	//might be safest to hard-code prefix, and then just store sub-range number in file...The the prefix is protected
	
	//-------------------------------------
	char uid[100];
	char subrange_uid[100];
	Read_SitePrefix_UID_File(subrange_uid, uid_fname);		//get the UID sub-range prefix from file
	
	//get the date prefix, or load it from a pre-existing file
	char date_prefix[64];
	GetDateCode(date_prefix, InFile_UID_Date_fname);
	
	int sub = atoi(subrange_uid);
	
	if(sub >= MIN_UID_SUB_RANGE && sub <= MAX_UID_SUB_RANGE)
	{	//set the UID here
		//our UID format is prefix.dro_prefix.lab_prefix.data_type.scan_code.slice
		sprintf(uid, "%s.%s.%s.%s.%s.%d", UID_prefix, subrange_uid, SOP_INSTANCE_UID, date_prefix, CT_UID_CODE, (slice + 1));
		
		DcmTagKey dct(DCM_SOPInstanceUID);
		ReplaceOrInsertString(dataset, &dct, &uid[0], dcmfile);
		DcmTagKey dct2(DCM_MediaStorageSOPInstanceUID);
		ReplaceOrInsertString(dataset, &dct2, &uid[0], dcmfile);
		
		sprintf(uid, "%s.%s.%s.%s", UID_prefix, subrange_uid, STUDY_INSTANCE_UID, date_prefix);
		dataset->putAndInsertString(DCM_StudyInstanceUID, uid);
		
		sprintf(uid, "%s.%s.%s.%s.%s", UID_prefix, subrange_uid, SERIES_INSTANCE_UID, date_prefix, CT_UID_CODE);
		dataset->putAndInsertString(DCM_SeriesInstanceUID, uid);
		
		sprintf(uid, "%s.%s.%s.%s", UID_prefix, subrange_uid, FRAMEOF_REFERENCE_UID, date_prefix);
		dataset->putAndInsertString(DCM_FrameOfReferenceUID, uid);
	 }
	 else
	 {
	 	//failure - we should abort
		printf("ERROR: UID sub-range prefix (%d) from file outside subrange!\n", sub);
	 	exit(1);
	 }
}
