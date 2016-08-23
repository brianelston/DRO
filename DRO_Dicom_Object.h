/*
 *  DRO_Dicom_Object.h
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/13/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#ifndef DRO_Dicom_Object_H
#define DRO_Dicom_Object_H

#include "DRO_DICOM_Header_PET.h"
#include "DRO_Dicom_Header_CT.h"
#include "DRO_Phantom_PET.h"
#include "DRO_Phantom_CT.h"
#include "InputParameters.h"

class DRO_Dicom_Object
{	//the composite dicom object of header data and pixel data
public:
	
	DRO_Dicom_Object();
	~DRO_Dicom_Object();
	
	bool GenerateDICOMStack(InputParams * in_par);
	
	bool ReadCommandParameterFile(char * filename);		//parse input file of command parameters
	void ShowParameters(bool braw_data);								//display runtime variables
	static void ShowHelp();								//show usage - static so can be called without instantiation

private:

	DRO_Object_Type DRO_Type;							//what type of simulation are we doing, emission or transmission
	DRO_Rawdata_Type RAW_type;

	bool SaveDICOMSLice ( int slice, char * Headerfname, char * uid_fname, char * InFile_UID_Date_fname, InputParams * in_par);		//prep a single slice in DICOM format with dicom header and call save
	bool SaveDICOMFile(DcmFileFormat *fileformat, char * fname);															//save a DICOM file
	
	bool LoadPixelDataFromDICOM(char * PixelData_folder, char * PixelData_fname);	//for testing of headers
	
	bool LoadPixelDataFromRAW(char * PixelData_folder, char * PixelData_fname, int x_dim, int y_dim, int z_dim, float scale);	//bfe 12.31.12 - for encapsulating raw ASIM data in a DICOM header format
	
	void GenerateFolder();													//generate folder for DICOM stack
	
	void AllocateClassItems();												//allocates virtual objects of correct class types

	bool PopulateCommandLineItemFromFile(DRO_Phantom_PET * obj, char * key, char * value);	//helper for reading command input file

	DRO_Phantom *		DRO_phantom_obj;	//pixel data, either EM or ATT
	
	DRO_DICOM_Header *	DRO_Header_obj;		//dicom header data - for building output file, either EM or ATT
	
	char OutputFileName[256];				//dicom file output name prefix - if user specified - optional
	char OutputFolderName[256];				//folder for dicom stack
	
	char InFile_DICOMDIR_fname[256];	

	//bfe 1.2.13 YAFF and STIR data file DICOM encapsulation mode functions
	bool File_Exists(char * filename);	
	bool IsYaff(char * fname);
	bool LoadYAFFRawAttributes(InputParams * in_par, char * PixelData_folder, char * PixelData_fname);
	bool ParseHeadernameFromFilename(const char * fname, char * hdrname);
  
	bool Read_YHDR_HeaderFile(InputParams * in_par, char * filename);
	char * GetNextLineYHDR(char * buf, int maxsize, FILE * fp);
	bool PopulateYHDRItemFromFile(InputParams * in_par, char * key, char * value);
};

/*
bfe 1.1.13 - Encapsulation mode usage:

Example command line parameters:

1)For ASIM -I image file input
-i EM_YAFF_Params.txt -h EM_YAFF_Header.txt -r arsene_I_f365m962.yaff

2) For STIR image file input
-i EM_YAFF_Params.txt -h EM_YAFF_Header.txt -x 265 -y 265 -z 63 -r arsene_Ehk_psf_nk_AW16.v

Use the appropriate command line witth the correct type of input file
A standard DRO header definition file is also required to perform correct DICOM header encapsulation
YAFF image file requires image data and .yhdr header file
STIR mode requires image file and command line parameter definition of image dimensions

Example parameter file:

DRO_Type=0 						#0 - emission, 1 - transmission
image_size_x=500.0
image_size_y=500.0
slice_thickness_z=2.0
#------------------FOLDER------------------
OutputFolderName=2012_12_31_arsene_yaff
#OutputFolderName=2012_12_31_arsene_STIR_test	#use with DRO param ex. '-i EM_YAFF_Params.txt -h EM_YAFF_Header.txt -x 265 -y 265 -z 63 -r arsene_Ehk_psf_nk_AW16.v'
#-------------------RESCALE & OTHERS------------------
Create_DICOMDIR=0			

Example DICOM header definition file:

#----------------PATIENT----------------
DCM_AcquisitionDate=20121231					#same as DCM_StudyDate
DCM_StudyDescription=(20121231) IRL Encapsulated Raw Pixel Data
DCM_SeriesDescription=(20121231) Encapsulated Raw Pixel data
DCM_PatientWeight=70
DCM_PatientSize=165
DCM_PatientName=RawPixelData					#Displayed name in Osirix
#----------------SPECIFIC-------------------
DCM_ImplementationClassUID=1.3.6.1.4.1.9590.100.1.0.100.4.0
DRO_PrivateTag=0020;4000=Copyright 2013 Brian Elston, Larry Pierce, and Paul Kinahan. University of Washington UWMC IRL.;LT
#----------------GENERIC----------------
DCM_SamplesPerPixel=1	
DCM_PixelRepresentation=1
#----------------DEFAULT----------------
DCM_ImageType=ORIGINAL\PRIMARY
DCM_Modality=PT
DCM_BitsAllocated=16
DCM_BitsStored=16
DCM_HighBit=15
DCM_SeriesType=WHOLE BODY\IMAGE
DCM_Units=BQML
DCM_CountsSource=EMISSION
DCM_DecayCorrection=START
DCM_DecayFactor=1
#----------------STUDY INSTANCE----------------
DCM_SOPClassUID=1.2.840.10008.5.1.4.1.1.128
DCM_MediaStorageSOPClassUID=1.2.840.10008.5.1.4.1.1.128		#same as DCM_SOPClassUID
DCM_TransferSyntaxUID=1.2.840.10008.1.2.1
#----------------RADIOPHARMACUITICAL----------------
DCM_ActualFrameDuration=1500000					#25 minute scan - 25* 60 * 1000
DCM_AcquisitionTime=140100.000000
#----------------RESCALE & SLOPE----------------
DCM_RescaleIntercept=0
#----------------ANATOMICAL----------------
DCM_RadiopharmaceuticalInformationSequence=[DCM_RadiopharmaceuticalStartTime=130100.000000:DCM_RadionuclideTotalDose=370000000:DCM_RadionuclideHalfLife=6586:DCM_RadionuclidePositronFraction=.9673]DCM_RadionuclideCodeSequence=[DCM_CodeValue=C-111A1:DCM_CodingSchemeDesignator=SRT:DCM_CodeMeaning=^18^Fluorine]


*/

#endif