/*
 *  DRO_DICOM_Header_PET.cpp
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/18/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#include "DRO_DICOM_Header_PET.h"

void DRO_DICOM_Header_PET::SetDICOMHeaderFields(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj, int slice, char * uid_fname, char * InFile_UID_Date_fname, DcmFileFormat * dcmfile, bool braw_data)
{	
	//Fill in header fields........................
	
	SetImageDataDcmInfo(dataset, DRO_phantom_obj);
	SetPixelSpacingDcmInfo(dataset, DRO_phantom_obj);
	SetSliceThicknessDcmInfo(dataset, DRO_phantom_obj);
	
	if(!braw_data)
	{
	  SetRescaleSlopeDcmInfo(dataset, slice, DRO_phantom_obj);
	}
	else
	{
	  SetRescaleSlopeDcmInfo_toHARDVALUE(dataset, slice, 1.0);
	}
	
	SetHeaderSliceInfo(dataset, slice, DRO_phantom_obj);
	
	//bfe 1.2.2013 set instance ID - choose appropriate DICOM header prefix
	if(!braw_data)
	{ //DRO slice data - not raw
	  SetInstitutionUIDDcmInfo(dataset, slice, uid_fname, InFile_UID_Date_fname, dcmfile, UID_DRO_PREFIX);
	}
	else
	{ //raw pixel data encapsulated UID
	  SetInstitutionUIDDcmInfo(dataset, slice, uid_fname, InFile_UID_Date_fname, dcmfile, UID_DRES_PREFIX);
	}
}
//--------------------------------------------------------
void DRO_DICOM_Header_PET::SetPixelSpacingDcmInfo(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj)
{	//number of slices and whatnot
	
	char temp[256] = {'\0'};
	char val1[256] = {'\0'};
	char val2[256] = {'\0'};
	
	sprintf(val1, "%f", DRO_phantom_obj->Get_dx());
	sprintf(val2, "%f", DRO_phantom_obj->Get_dy());
	RemoveLeadingZeros(val1);
	RemoveLeadingZeros(val2);
	sprintf(temp, "%s\\%s", val1, val2);
	
	dataset->putAndInsertString(DCM_PixelSpacing, temp);	
}
//--------------------------------------------------------
void DRO_DICOM_Header_PET::SetRescaleSlopeDcmInfo_toHARDVALUE(DcmDataset *dataset, int slice, float value)
{
	char temp[256] = {'\0'};
	sprintf(temp, "%.15lf", value);	//16 characters, 14 decimal plus '0.'
	RemoveLeadingZeros(&temp[0]);
	dataset->putAndInsertString(DCM_RescaleSlope, temp);
}
//--------------------------------------------------------
void DRO_DICOM_Header_PET::SetRescaleSlopeDcmInfo(DcmDataset *dataset, int slice, DRO_Phantom * DRO_phantom_obj)
{
	char temp[256] = {'\0'};
	//OFString retval = NULL;
	
	//fdg stuff
	//todo: test this exists in file, else through an error!
	//todo: dissallow image centric fields to be set by user
	if(!dataset->tagExistsWithValue(DCM_RadionuclideHalfLife, OFTrue))
	{
		printf("ERROR (SetRescaleSlopeDcmInfo): Cannot retrieve required tag from header file: DCM_RadionuclideHalfLife!\n");
		return;
	}	
	if(!dataset->tagExistsWithValue(DCM_ActualFrameDuration, OFTrue))
	{
		printf("ERROR (SetRescaleSlopeDcmInfo): Cannot retrieve required tag from header file: DCM_ActualFrameDuration!\n");
		return;
	}
	if(!dataset->tagExistsWithValue(DCM_AcquisitionTime, OFTrue))
	{
		printf("ERROR (SetRescaleSlopeDcmInfo): Cannot retrieve required tag from header file: DCM_AcquisitionTime!\n");
		return;
	}
	if(!dataset->tagExistsWithValue(DCM_RadiopharmaceuticalStartTime, OFTrue))
	{
		printf("ERROR (SetRescaleSlopeDcmInfo): Cannot retrieve required tag from header file: DCM_RadiopharmaceuticalStartTime!\n");
		return;
	}
	if(!dataset->tagExistsWithValue(DCM_RadionuclideTotalDose, OFTrue))
	{
		printf("ERROR (SetRescaleSlopeDcmInfo): Cannot retrieve required tag from header file: DCM_RadionuclideTotalDose!\n");
		return;
	}
	if(!dataset->tagExistsWithValue(DCM_PatientWeight, OFTrue))
	{
		printf("ERROR (SetRescaleSlopeDcmInfo): Cannot retrieve required tag from header file: DCM_PatientWeight!\n");
		return;
	}
	
	double half_life = atof(FindAndGetSequencedItemOFString(dataset, DCM_RadiopharmaceuticalInformationSequence, DCM_RadionuclideHalfLife).c_str());	//half life

	//This patient is scanned one hour after injection: 
	double lambda = (log(2))/half_life;	///half life
	double Tfd = atof(FindAndGetItemOFString(dataset, DCM_ActualFrameDuration).c_str());
	
	double frameref = floor((1/lambda)*log((lambda*Tfd)/(1-exp(-lambda*Tfd))));
	sprintf(temp, "%d", (int) frameref); //should be an int
	dataset->putAndInsertString(DCM_FrameReferenceTime, temp);	
		
	OFString acqval = NULL;
	OFString startval = NULL;
	dataset->findAndGetOFString(DCM_AcquisitionTime, acqval);
	startval = FindAndGetSequencedItemOFString(dataset, DCM_RadiopharmaceuticalInformationSequence, DCM_RadiopharmaceuticalStartTime);
	
	double decay_hrs = atof((((string) ((char *)acqval.c_str ( ))).substr(0, 2)).c_str()) - atof((((string) ((char *)startval.c_str ( ))).substr(0, 2)).c_str()); //needs crazy cast
	double decay_mins = atof((((string) ((char *)acqval.c_str ( ))).substr(2, 2)).c_str()) - atof((((string) ((char *)startval.c_str ( ))).substr(2, 2)).c_str()); //needs crazy cast
	double decay_secs = atof((((string) ((char *)acqval.c_str ( ))).substr(4, 2)).c_str()) - atof((((string) ((char *)startval.c_str ( ))).substr(4, 2)).c_str()); //needs crazy cast
	double decay_msec = atof((((string) ((char *)acqval.c_str ( ))).substr(7, 6)).c_str()) - atof((((string) ((char *)startval.c_str ( ))).substr(7, 6)).c_str()); //needs crazy cast

	double elapsed_time = decay_hrs*3600 + decay_mins*60 + decay_secs + (decay_msec*pow(10, -6));
	
	double total_dose = atof(FindAndGetSequencedItemOFString(dataset, DCM_RadiopharmaceuticalInformationSequence, DCM_RadionuclideTotalDose).c_str());	//total dose
	
	double corr_dose = total_dose * pow(0.5, elapsed_time / half_life);
	
	//be 1.10.12 - typically therescale slope applies to the entire image - in the GE variant it only applys to the maximum value in that image slice.
	//we can scale the value based on the max value in a given slice proportional to the max value in the entire image (max_float)
	//then we find the max value in that slice, and scale the rescale slope based on the proportional value to entire images maximum value

	double max_slice_float = DRO_phantom_obj->Calculate_Max_Float_For_Slice(slice);
	//note this max_float for the slice is float 0-1
	
	///do the rescale slope
	double rescale_slope;
	if(!DRO_phantom_obj->Rescale_Slope_Per_Slice)	//if !GE
	{	rescale_slope = (DRO_phantom_obj->Get_max_float() * corr_dose)/(MAX_IMAGE_VALUE * atof(FindAndGetItemOFString(dataset, DCM_PatientWeight).c_str()) * 1000.0);
	}
	else //if GE
	{	//rescale_slope = (max_slice_float * DRO_phantom_obj->Get_max_float() * corr_dose)/(MAX_IMAGE_VALUE * atof(FindAndGetItemOFString(dataset, DCM_PatientWeight).c_str()) * 1000.0);
		rescale_slope = (max_slice_float * corr_dose)/(MAX_IMAGE_VALUE * atof(FindAndGetItemOFString(dataset, DCM_PatientWeight).c_str()) * 1000.0);
	}
	
	sprintf(temp, "%.15lf", rescale_slope);	//16 characters, 14 decimal plus '0.'
	RemoveLeadingZeros(&temp[0]);
	dataset->putAndInsertString(DCM_RescaleSlope, temp);
}
//--------------------------------------------------------
void DRO_DICOM_Header_PET::SetHeaderSliceInfo(DcmDataset *dataset, int slice, DRO_Phantom * DRO_phantom_obj)
{	//slice info
	char temp[256] = {'\0'};
	sprintf(temp, "%d", (slice+1));
	dataset->putAndInsertString(DCM_InstanceNumber, temp);
	
	dataset->putAndInsertString(DCM_ImageIndex, temp);
	
	if(DRO_phantom_obj->Get_z_center_vals()[slice] == (int) DRO_phantom_obj->Get_z_center_vals()[slice])	//then an int
	{	
		sprintf(temp, "%d", (int) DRO_phantom_obj->Get_z_center_vals()[slice]);
		dataset->putAndInsertString(DCM_SliceLocation, temp);
		
		sprintf(temp, "%.7f\\%.7f\\%d", DRO_phantom_obj->Get_x_center_vals()[0], -DRO_phantom_obj->Get_y_center_vals()[0], (int) DRO_phantom_obj->Get_z_center_vals()[slice]);
		dataset->putAndInsertString(DCM_ImagePositionPatient, temp);
	}
	else	//encode as float
	{	sprintf(temp, "%f", DRO_phantom_obj->Get_z_center_vals()[slice]);
		dataset->putAndInsertString(DCM_SliceLocation, temp);
		
		sprintf(temp, "%.7f\\%.7f\\%f", DRO_phantom_obj->Get_x_center_vals()[0], -DRO_phantom_obj->Get_y_center_vals()[0], DRO_phantom_obj->Get_z_center_vals()[slice]);
		dataset->putAndInsertString(DCM_ImagePositionPatient, temp);
	}
	
	sprintf(temp, "%d", DRO_phantom_obj->Get_num_dz());
	dataset->putAndInsertString(DCM_NumberOfSlices, temp);
}
//--------------------------------------------------------
void DRO_DICOM_Header_PET::SetInstitutionUIDDcmInfo(DcmDataset *dataset, int slice, char * uid_fname, char * InFile_UID_Date_fname, DcmFileFormat * dcmfile, char * UID_prefix)
{
	//set any unique UID's...
	//bfe 9.22.11 - need to read prefix from a read-only UID file - the UID prefix needs to be assigned
	//might be safest to hard-code prefix, and then just store sub-range number in file...Then the rest of the prefix is protected
	
	//-------------------------------------
	char uid[100];
	char subrange_uid[100];
	Read_SitePrefix_UID_File(subrange_uid, uid_fname);		//get the UID sub-range prefix from file - a single digit number
	
	//get the date prefix, or load it from a pre-existing file
	char date_prefix[64];
	GetDateCode(date_prefix, InFile_UID_Date_fname);
	
	int sub = atoi(subrange_uid);
	//150.IRL.DRO.site.field_identifier.timestamp.pet/ct.slice
	
	if(sub >= MIN_UID_SUB_RANGE && sub <= MAX_UID_SUB_RANGE)
	{	
		//our UID format is prefix.dro_prefix.lab_prefix.data_type.scan_code.slice
		//for DRO use this prefix - pass in to function
		sprintf(uid, "%s.%s.%s.%s.%s.%d", UID_prefix, subrange_uid, SOP_INSTANCE_UID, date_prefix, PET_UID_CODE, (slice + 1));
		
		DcmTagKey dct(DCM_SOPInstanceUID);
		ReplaceOrInsertString(dataset, &dct, &uid[0], dcmfile);
		DcmTagKey dct2(DCM_MediaStorageSOPInstanceUID);
		ReplaceOrInsertString(dataset, &dct2, &uid[0], dcmfile);
		
		sprintf(uid, "%s.%s.%s.%s", UID_prefix, subrange_uid, STUDY_INSTANCE_UID, date_prefix);
		dataset->putAndInsertString(DCM_StudyInstanceUID, uid);
		
		sprintf(uid, "%s.%s.%s.%s.%s", UID_prefix, subrange_uid, SERIES_INSTANCE_UID, date_prefix, PET_UID_CODE);
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
