/*
 *  DRO_Dicom_Object.cpp
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/13/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *  Main DRO Dicom generation object - is both pixel and header data
 */

#include "DRO_Dicom_Object.h"

#include "dcmtk/dcmdata/dcddirif.h"  
#include <float.h>

//--------------------------------------------------------
DRO_Dicom_Object::DRO_Dicom_Object()
{	//constructor
	DRO_phantom_obj = NULL;
	DRO_Header_obj = NULL;
	
	OutputFileName[0] = '\0';
	OutputFolderName[0] = '\0';
	
	DRO_Type = DRO_EMISSION;		//default is emission
	
	strcpy(InFile_DICOMDIR_fname, DICOMDIR_DEFAULT_FNAME);
	RAW_type = RAW_STIR;
}
//--------------------------------------------------------
DRO_Dicom_Object::~DRO_Dicom_Object()
{	//destructor
	if(DRO_phantom_obj != NULL)
	{	delete(DRO_phantom_obj);
		DRO_phantom_obj = NULL;
	}
	if(DRO_Header_obj != NULL)
	{	delete(DRO_Header_obj);
		DRO_Header_obj = NULL;
	}
}
//--------------------------------------------------------
void DRO_Dicom_Object::AllocateClassItems()
{
	//allocate virtual classes, mechanism for automatic function call signatures through inheritance
	if(DRO_Type == DRO_EMISSION)
	{
		DRO_phantom_obj = new DRO_Phantom_PET();
		DRO_Header_obj = new DRO_DICOM_Header_PET();
		
		if(	OutputFolderName[0] == '\0')		//if not set by user, then use a default
		{	strcpy(OutputFolderName, EMISSION_FOLDERNAME_DEFAULT);
		}
	}
	else	//transmission - attenuation
	{
		DRO_phantom_obj = new DRO_Phantom_CT();
		DRO_Header_obj = new DRO_DICOM_Header_CT();
	
		if(	OutputFolderName[0] == '\0')		//if not set by user, then use a default
		{	strcpy(OutputFolderName, TRANSMISSION_FOLDERNAME_DEFAULT);
		}
	}
	
	if(DRO_phantom_obj == NULL || DRO_Header_obj == NULL)
	{
		printf("Error in allocation (AllocateClassItems:2)! Aborting!\n");
		this->~DRO_Dicom_Object();	//dealloc through destructor
		exit(1);
	}
}
//--------------------------------------------------------
bool DRO_Dicom_Object::IsYaff(char * fname)
{
  if(strstr(fname, ".yaff") != NULL)
  {	
	RAW_type = RAW_YAFFIMAGE; //set the detection flag
	return true;
  }
  return false;
}
//--------------------------------------------------------
bool DRO_Dicom_Object::ParseHeadernameFromFilename(const char * fname, char * hdrname)
{
	/*parse out the name for the header file, allocate and open
	Here we create a filename and open the FILE pointer for the mainheader file new to .YAFF
	strip off the filename extension and replace with appropriate header extension
	note hdrname must be previously allocated, or it wont be persistent*/
	  
	char *suffix = NULL;

	strcpy (hdrname, fname);	  
	suffix = (char*) strrchr(hdrname, '.');
	
	if (suffix) 
 	{
	  suffix[0] = '\0';
 	} 
 	
 	strcat(hdrname, ".yhdr");	/*append file extension*/
	
	return true;
}
//--------------------------------------------------------
bool DRO_Dicom_Object::LoadYAFFRawAttributes(InputParams * in_par, char * PixelData_folder, char * PixelData_fname)
{
	//bfe 1.2.2013
  	//YAFF image data 16 bit big endian unsigned
	
	//first get the .yhdr name - then check both it and the .yaff exists
	char header_fname[256];
	ParseHeadernameFromFilename(PixelData_fname, header_fname);
	
	if(!File_Exists(PixelData_fname) || !File_Exists(header_fname))
	{
		printf("DRO_phantom::LoadYAFFRawAttributes: Error opening yaff or yhdr file: %s or %s for reading!\n", PixelData_fname, header_fname);
		exit(1);
	}
	
	Read_YHDR_HeaderFile(in_par, header_fname);
	
	return true;
}
//--------------------------------------------------------
bool DRO_Dicom_Object::GenerateDICOMStack(InputParams * in_par)
{
	//Generate the entire stack of DICOM slices - THE MAIN WORKHORSE
	//if the input parameter file is .yaff, then read the .yhdr and get the stats for x,y,z from there
	if(in_par->b_RawDataInput && IsYaff(in_par->InFilePixelData_fname))
	{
	  //bfe 1.4.14 - then read in .yhdr and look for x, y, z
	  LoadYAFFRawAttributes(in_par, in_par->InFolderPixelData, in_par->InFilePixelData_fname);
	}
	
    DRO_phantom_obj->InitAndAllocate(in_par->b_RawDataInput, in_par->x_raw, in_par->y_raw, in_par->z_raw);	//allocate memory and set initial vars
   
	//do the work of building DRO from geometric data - original intent of this program - since expanded as a DICOM encapsulator
	if(in_par->b_RawDataInput == false)				  //bfe 12.31.12 if no pre-existing data, then calculate from the geometery
	{	//default is to generate the pixel data new	- from geometery
	
		DRO_phantom_obj->CreateGeometry();					//call to create emission/transmission pixel vals from geometry, set values as specified for activity or attenuation
	
		DRO_phantom_obj->ConvertImageDataToOutputFormat();	//convert to format (int 16) for output, also add noise if needed
	}
	else
	{
	  //CT should be ok now - bfe 1.2.2013
	  LoadPixelDataFromRAW(in_par->InFolderPixelData, in_par->InFilePixelData_fname, in_par->x_raw, in_par->y_raw, in_par->z_raw, in_par->Bq_raw_scalefactor);
	  
	  DRO_phantom_obj->ConvertImageDataToOutputFormat();	//convert to format (int 16) for output, also add noise if needed
	}
	
	GenerateFolder();									//generate new folders for dicom images
	
	if(in_par->bOutputDataBlock)
	{  	//NOTE: verification only, not part of the DICOM stack
	  	DRO_phantom_obj->WriteDROfile();			//and output the data block file, outputs just image data
	 }
	
	if(!DRO_phantom_obj->b_Create_DICOMDIR)	//no DICOMDIR - normal mode - bfe 1.11.2012
	{
		for(int j = 0; j < DRO_phantom_obj->Get_num_dz(); j++)
		{	//write the slices out as data blocks, as then we can use DCMTK to create a dicom file
			SaveDICOMSLice(j, in_par->InFileheaderFields_fname, in_par->InFileUID_fname, in_par->InFile_UID_Date_fname, in_par);
		}
	}
	else	//make the DICOMDIR, and whatever else is associated
	{
		//and if its a link file, we need to append to the existing dicom directory (do we need a command line option for the DICOMDIR filename?)
		//lets make a dicom directory too - simpler if we create on the fly
		DicomDirInterface dicomdir;
		OFCondition status;
			
		if(in_par->InFile_UID_Date_fname[0] != '\0')
		{	//if were linking to a file, then its the user's responsibility to make sure the previous dicomdir exists
			status = dicomdir.appendToDicomDir(DicomDirInterface::AP_GeneralPurpose, InFile_DICOMDIR_fname);
		}
		else
		{
			status = dicomdir.createNewDicomDir(DicomDirInterface::AP_GeneralPurpose, InFile_DICOMDIR_fname);	///E_ApplicationProfile->AP_GeneralPurpose	//DicomDirInterface::Ap_Default
		}
		
		if (!status.good())
		{
			printf("Error in creating DICOM Directory (GenerateDICOMStack:1)! Aborting!\n");
			this->~DRO_Dicom_Object();	//dealloc through destructor
			exit(1);
		}
		
		char fname_directory[256];	
		
		for(int j = 0; j < DRO_phantom_obj->Get_num_dz(); j++)
		{	//write the slices out as data blocks, as then we can use DCMTK to create a dicom file
			SaveDICOMSLice(j, in_par->InFileheaderFields_fname, in_par->InFileUID_fname, in_par->InFile_UID_Date_fname, in_par);
			//either need the name returned, or the dicomdir passed in
			
			sprintf(fname_directory, "%.6d", (j+1));	//filename is different from that of saved filename in dicom directory - so do it here
			dicomdir.addDicomFile( fname_directory,  OutputFolderName);	//add the filename to the dicom directory, could follow with optional directory parameter
		}
		
		//also generate the DICOMDIR structure here! bfe 1.9.2012
		status = dicomdir.writeDicomDir();
		if (status.bad())
		{	 cerr << "Error: cannot write DICOMDIR (" << status.text() << ")" << endl;
	   
		}
	}
	
	return true;
}
//--------------------------------------------------------
bool DRO_Dicom_Object::SaveDICOMSLice ( int slice, char * Headerfname, char * uid_fname, char * InFile_UID_Date_fname, InputParams * in_par)
{	//save an individual slice - loading all the DICOM fields first

	DcmFileFormat fileformat;						//our DCMTK file
	DcmDataset *dataset = fileformat.getDataset();	//new DCMTK blank dataset, associate with a file
	
	char fname[256];		
	
	if(OutputFileName[0] != '\0')	//utilize user specified filename & folder if desired
	{
		if(!DRO_phantom_obj->b_Create_DICOMDIR)	//DICOM files with a DICOMDIR cannot have the .dcm extension! (or any extension)
		{	sprintf(fname, "%s/%s%.6d.dcm", OutputFolderName, OutputFileName, (slice+1));
		}
		else
		{	sprintf(fname, "%s/%s%.6d", OutputFolderName, OutputFileName, (slice+1));
		}
	}
	else
	{
		if(!DRO_phantom_obj->b_Create_DICOMDIR)
		{	sprintf(fname, "%s/%.6d.dcm", OutputFolderName, (slice+1));
		}
		else
		{	sprintf(fname, "%s/%.6d", OutputFolderName, (slice+1));
		}
	}
	
	//PROCESS- 
	//1) load header file as initialization
	//2) Set parameters input based and/or requiring calculation based on parameters
	//3) Reload header file, in order to overwrite parameters if user desires; 
	//Note some parameters we dont allow the user to set (e.x. MediaSOPInstanceUID)
	
	//1)read from header file for any initialization - note we do this twice, before and after the in code value
	DRO_Header_obj->ReadHeaderFieldsFileAndSet(dataset, Headerfname, &fileformat);
	
	//lets follow the LP file order & detail
	//2)Fill in header fields based on inpput details
	DRO_Header_obj->SetDICOMHeaderFields(dataset, DRO_phantom_obj, slice, uid_fname, InFile_UID_Date_fname, &fileformat, in_par->b_RawDataInput);
	
	//3)and then set from the header file to overwrite any required changes...file takes precedene over internal if set
	DRO_Header_obj->ReadHeaderFieldsFileAndSet(dataset, Headerfname, &fileformat);
	
	//insert the pixel data in the dicom file
	if(!DRO_phantom_obj->SetDICOMPixelData(dataset, slice))
	{	return false;
	}
	
	//dataset->computeGroupLengthAndPadding(EGL_withGL, EPD_withPadding, EXS_LittleEndianExplicit, EET_ExplicitLength);	//set the correct encoding
	//bfe - 10.28.11 - we want explicit length encoding for sequences and items, not undefined length.....???
	dataset->calcElementLength( EXS_LittleEndianExplicit, EET_ExplicitLength);
	
	//save the dicom file
	if(!SaveDICOMFile(&fileformat, fname))
	{	return false;
	}

	return true;
}
//--------------------------------------------------------
bool DRO_Dicom_Object::SaveDICOMFile(DcmFileFormat *fileformat, char * fname)
{	//save a DICOM dataset to file
	OFCondition status = fileformat->saveFile(fname, EXS_LittleEndianExplicit);	//little endian implicit possibly...
		
	if (status.bad())
  	{	cerr << "Error: cannot write DICOM file (" << status.text() << ")" << endl;
		return false;
	}
	return true;
}
//--------------------------------------------------------
void DRO_Dicom_Object::GenerateFolder()
{	
	//NOTE: Currently only MAC/Unix tested - needs win testing but should be fine	//#ifdef _WIN32
	// make folder - if it does not exist - cleanup first
	char cmd[256];
	
	if (access(OutputFolderName, F_OK) == 0)
	{	//if folder exists, remove it
	  	sprintf(cmd, "rm -r %s", OutputFolderName);
	  	system(cmd);
	}
	//and make it fresh
	sprintf(cmd, "mkdir %s", OutputFolderName);
	system(cmd);
}
//--------------------------------------------------------
#include <algorithm> //required for std::swap

#define ByteSwap5(x) ByteSwap((unsigned char *) &x,sizeof(x))
//--------------------------------------------------------
void ByteSwap(unsigned char * b, int n)
{
   register int i = 0;
   register int j = n-1;
   while (i<j)
   {
      std::swap(b[i], b[j]);
      i++, j--;
   }
}
//--------------------------------------------------------
bool DRO_Dicom_Object::LoadPixelDataFromRAW(char * PixelData_folder, char * PixelData_fname, int x_dim, int y_dim, int z_dim, float scale)
{
	//bfe 1.2.2013
  	//read in raw data file and set the appropriate data structure to allot for these values
	double maxval = -1000000;//get the max value for setting the scaling later on
	  
	FILE* fdes = NULL;
	if ((fdes = fopen(PixelData_fname,"r")) == NULL)
	{
		printf("DRO_phantom::LoadPixelDataFromRAW: Error opening %s for reading!\n", PixelData_fname);
		exit(1);
	}
	
	if(RAW_type == RAW_STIR)  //STIR mode
	{
	  printf("RAW DATA MODE: Input file must be 32 bit float litte endian PET/CT data ONLY!\n");
	  
	  float* rawdata = (float*) malloc(sizeof(float) * DRO_phantom_obj->Get_num_dro());		
	  memset(rawdata, 0, (sizeof(float) * DRO_phantom_obj->Get_num_dro()));	
	  
	  int result = fread (rawdata, sizeof(float), DRO_phantom_obj->Get_num_dro(), fdes);
	  if (result != DRO_phantom_obj->Get_num_dro()) {fputs ("Reading error",stderr); exit (3);}
	  
	  for(int i = 0; i < DRO_phantom_obj->Get_num_dro(); i++)
	  {
		if(rawdata[i] > maxval)
		{	maxval = rawdata[i];
		}
		
		DRO_phantom_obj->DRO_object[i] = rawdata[i] * scale;	//add scaling
	  }
	  
	  free(rawdata);
	}
	else if(RAW_type == RAW_YAFFIMAGE)	//-I ASIM image mode
	{
	  printf("YAFF IMAGE MODE: Input file must be 16 bit unsigned big endian ONLY!\n");
	  unsigned short * rawdata = (unsigned short *) malloc(sizeof(unsigned short ) * DRO_phantom_obj->Get_num_dro());		
	  memset(rawdata, 0, (sizeof(unsigned short ) * DRO_phantom_obj->Get_num_dro()));	
	  
	  int result = fread (rawdata, sizeof(unsigned short ), DRO_phantom_obj->Get_num_dro(), fdes);
	  if (result != DRO_phantom_obj->Get_num_dro()) {fputs ("Reading error",stderr); exit (3);}
	   
	  for(int i = 0; i < DRO_phantom_obj->Get_num_dro(); i++)
	  {		
			unsigned short in_val = rawdata[i];
			
			ByteSwap5(in_val);	//big endian to little endian
		
			if(in_val > maxval)
			{	maxval = in_val;
			}
			
			DRO_phantom_obj->DRO_object[i] = in_val * scale;	//convert to double little endian add scaling - likely needs some sort of byte value correction
	  }
	  
	  free(rawdata);
	}
	else
	{
	  printf("UNSUPPORTED RAW IMAGE MODE: Input file must be 16 bit unsigned big endian YAFF or 32 bit float little endian STIR ONLY! Aborting\n");
	  fclose(fdes);	
	  exit(1);	  
	}
	
	printf("DRO_phantom::read file %s\n", PixelData_fname);
	fclose(fdes);	

	//and need to set the max_float, jpreviously found
	DRO_phantom_obj->Set_max_float(maxval);
	return true;
}
//--------------------------------------------------------
bool DRO_Dicom_Object::LoadPixelDataFromDICOM(char * PixelData_folder, char * PixelData_fname)
{
	//load pixel data from prexisting Dicom slices
	//for internal testing of DICOM header fields...without needing to recalculate the geometry (ie re-use from existing dicom file)
	DcmFileFormat fileformat;
	char fname[256];
	
	for(int j = 0; j < DRO_phantom_obj->Get_num_dz(); j++)
	{	//write the slices out as data blocks, as then we can use DCMTK to create a dicom file
		
		if(PixelData_fname[0] == '\0')
			sprintf(fname, "%s/%.6d.dcm", PixelData_folder, j+1);
		else
			sprintf(fname, "%s/%s%.6d.dcm", PixelData_folder, PixelData_fname, j+1);
			
		OFCondition status = fileformat.loadFile(fname);
			
		if (status.bad())
		{	std::cerr << "Error: cannot load DICOM file(" << status.text() << ") : filename = "<< fname << std::endl;
			return false;
		}
		DcmDataset * lDataset = fileformat.getDataset();
		if(!DRO_phantom_obj->GetDICOMPixelData(lDataset, j))
		{
			return false;
		}
	}
	
	//and need to set the max_float, just grab from users input command parameter file
	DRO_phantom_obj->Calculate_Max_Float();
	return true;
}
//--------------------------------------------------------
void DRO_Dicom_Object::ShowParameters(bool braw_data)
{	
  //show runtime parameters
  printf("\nCommand parameters for %s DRO run:\n", APP_NAME);
  
  if(!braw_data)
  {
	if(DRO_Type == DRO_EMISSION)
	{
	  printf("------- Emission Image Run------------\n");
	  printf("Body activity is : %f\n", DRO_phantom_obj->body_activity);
	  printf("Center rod activity is : %f\n", DRO_phantom_obj->center_rod_activity);
	  printf("Sphere activity is : %f\n", DRO_phantom_obj->sphere_activity);
	  printf("Background activity is : %f\n", DRO_phantom_obj->background_activity);
	  
	  if(DRO_phantom_obj->b_ShowHotVoxel)
	  {	printf("Hot voxel activity is : %f\n", DRO_phantom_obj->hot_voxel_activity);	
		  printf("Cold voxel activity is : %f\n", DRO_phantom_obj->negative_voxel_activity);
	  }
	  else
	  {	printf("Hot and cold voxel shall NOT be shown.\n");
	  }
	}
	else	//transmission
	{
	  printf("------- Transmission Image Run------------\n");
	  printf("Body attenuation is : %f\n", DRO_phantom_obj->body_attenuation);
	  printf("Center rod attenuation is : %f\n", DRO_phantom_obj->center_rod_attenuation);
	  printf("Sphere attenuation is : %f\n", DRO_phantom_obj->sphere_attenuation);
	  printf("Shell activity is : %f\n", DRO_phantom_obj->shell_attenuation);
	  printf("Background activity is : %d\n", TRANSMISSION_BACKGROUND_VALUE);
	}
	printf("\n");
	//all runs
  
	printf("Voxel fine division sampleing is : %d\n", DRO_phantom_obj->num_fine);
	
	if(DRO_phantom_obj->b_show_test_pattern && DRO_Type == DRO_EMISSION)
	{		
		  printf("Test pattern (checkerboard) shall be applied, centered on slice %d.\n", DRO_phantom_obj->test_pattern_slice+1);
		  printf("Test pattern area centerpoint x, y: %f, %f.\n", DRO_phantom_obj->test_pattern_area_centerx, DRO_phantom_obj->test_pattern_area_centery);
		  printf("Test pattern volume centerpoint x, y: %f, %f.\n", DRO_phantom_obj->test_pattern_volume_centerx, DRO_phantom_obj->test_pattern_volume_centery);
		  printf("Test pattern extent (x, y,) %f; (z) %f.\n", DRO_phantom_obj->test_pattern_extentxy, DRO_phantom_obj->test_pattern_volume_extentz);
	}
	else
	{		printf("Image will be created without test pattern.\n");
	}
	  
	if(DRO_phantom_obj->b_CalculateNoise && DRO_Type == DRO_EMISSION)
	{		
		  if(DRO_phantom_obj->b_GaussianSmooth)
		  {	printf("Noise and smoothing shall be applied.\n");
			  printf("FWHM of Gaussian smoothing is : %.9f\n", DRO_phantom_obj->smooth_fwhm);
		  }
		  else
		  {	printf("Noise shall be applied, without smoothing.\n");
		  }
		  
		  #ifdef MERSENNE_RAND
			  printf("The Mersenne-Twister psuedo-random generator shall be used.\n");
		  #else
			  printf("The C default random() psuedo-random generator shall be used.\n");
		  #endif
		  
		  //printf("Noisy slice is : %d\n", DRO_phantom_obj->noisy_slice+1);
		  printf("Amount of noise is : %.9f\n", DRO_phantom_obj->AmountNoise);
		  printf("Pixel mean variation added by noise is : %d\n", int(DRO_phantom_obj->AmountNoise * MAX_IMAGE_VALUE));
		  
		  //if noise is applied to the test voxel slice or a test pattern slice, warn user this is not good
	}
	else
	{		printf("Noiseless image will be created.\n");
	}
  }
  else
  {
	printf("RAW DATA MODE! error checking disabled!\n");
  }
  
  printf("Output filename prefix is : %s\n", OutputFileName);
  
  printf("Number of x pixels are : %d\n", DRO_phantom_obj->num_x_pixels);
  printf("Number of y pixels are : %d\n", DRO_phantom_obj->num_y_pixels);
  
  printf("Image size in x is %f mm\n", DRO_phantom_obj->image_size_x);
  printf("Image size in y is %f mm\n", DRO_phantom_obj->image_size_y);
  
  printf("Image size in z is %f mm\n", (DRO_phantom_obj->image_size_half_z * 2.0));
  printf("Total number of slices are %d\n", (int)((DRO_phantom_obj->image_size_half_z * 2.0) / DRO_phantom_obj->dz));
  
  printf("Size of x pixels are : %f\n", DRO_phantom_obj->image_size_x/(double)DRO_phantom_obj->num_x_pixels);
  printf("Size of y pixels are : %f\n", DRO_phantom_obj->image_size_y/(double)DRO_phantom_obj->num_y_pixels);
  printf("Slice thickness in z is : %f mm\n", DRO_phantom_obj->dz);
  
  printf("Folder for output is : %s\n", OutputFolderName);

  printf("------------------------------\n\n"); 
}
//--------------------------------------------------------
void DRO_Dicom_Object::ShowHelp()
{
  static bool ShowHelpAlready = false;
  
  if(ShowHelpAlready == false)	//only do once per run
  {
  	ShowHelpAlready = true;
  }
  else
  {
  	return;;
  }

  printf("\nUsage for %s:\n\n", APP_NAME);
  printf("%s \n\n", APP_NAME);
  
  printf("--------Command line parameters------------\n");
  printf("[-d output data block \n");
  printf("[-i input command parameter filename <char: 256 max>]  \n");
  printf("[-h input header parameter filename <char: 256 max>]  \n");
  printf("[-u input UID link filename : this dicom file is used for the UID timestamp <char: 256 max>]  \n");
  printf("\n");
  
  printf("--------For DICOM Raw pixel data Encapsulation - command line parameters------------\n");
  printf("-r input_image_filename  \n");
  printf("-x input_image_Xdimension  \n");
  printf("-y input_image_Ydimension  \n");
  printf("-z input_image_Zdimension  \n");
  printf("-s input_image_scaling factor [0-1.0]  \n");
 // printf("[-p pixel data DICOM folder <char: 256 max>: optional folder for pre-generated pixel data]  \n");
  //printf("[-q pixel data filename <char: 256 max>: optional pre-generated DICOM pixel data]  \n");
  printf("\n");
  
  printf("--------File command input parameters------------\n");
  printf("--------For All Images (no parameters required)------------\n");
  printf("[DRO_Type: 0 for emission, 1 for transmission <int>]  \n");
  printf("\n");
  
  printf("[num_fine: number fine sampleing <int>]  \n");
  printf("[num_x_pixels: number x pixels <int>]  \n");
  printf("[num_y_pixels: number y pixels <int>]  \n");
  printf("[slice_thickness_z: z slice thickness in mm <double>]  \n");
  printf("[image_size_x: image size in x in mm <double>]  \n");
  printf("[image_size_y: image size in y in mm <double>]  \n");
  printf("[image_size_z: image size in z in mm <double>]  \n");
  printf("\n");
  
  printf("[OutputFileName: optional output filename prefix <char: 256 max>]  \n");
  printf("[OutputFolderName: folder to save DICOM stack in <char: 256 max>]  \n");
  printf("Use '#' to denote a comment (per line)\n");
  printf("\n");
  
  printf("--------For Emission Image------------\n");
  printf("[background_activity: background activity <int, char, double>]  \n");
  printf("[body_activity: body activity <int, char, double>]  \n");
  printf("[center_rod_activity: center rod activity <double>]  \n");
  printf("[sphere_activity: sphere activity <double>]  \n");
  printf("[hot_voxel_activity: hot pixel acivity  - slice 40 <double>]  \n");
  printf("[negative_voxel_activity: cold pixel acivity  - slice 40 <double>]  \n");
  printf("[show_hot_voxel: 0 for no hot voxel, 1 for hot voxel <int>]  \n");
  printf("\n");
  
  printf("[show_test_pattern: 0 for no pattern, 1 for pattern area and volume <int> (emission only)]  \n");
  printf("[test_pattern_slice: test pattern area and volume slice center <int> \n");
  printf("[test_pattern_area_centerxy: test pattern area center x,y in mm <double>,<double> \n");
  printf("[test_pattern_extentxy: test pattern area and volume extent x and y in mm <double> \n");
  printf("[test_pattern_volume_centerxy: test pattern volume center x,y in mm <double>,<double> \n");
  printf("[test_pattern_volume_extentz: test pattern volume extent z in mm <double> \n");
  printf("\n");
  
  printf("--------For Transmission Image------------\n");
  printf("[body_attenuation: body attenuation <double>]  \n");
  printf("[shell_attenuation: shell attenuation <double>]  \n");
  printf("[sphere_attenuation: sphere attenuation <double>]  \n");
  printf("[center_rod_attenuation: center rod attenuation <double>]  \n");
  printf("\n");
  //does transmision need a background value (other than -1000)?
}
//-------------------------------------------------------------------------------------
bool DRO_Dicom_Object::PopulateCommandLineItemFromFile(DRO_Phantom_PET * obj, char * key, char * value)
{ /*Command parameters that control type & variation of the run*/

  char       *prefixes;

  //--------------Phantom chararacteristics---------------
  if(strcmp(key, "body_activity") == 0)
  {
  	obj->body_activity = atof(value);
  }
  else if(strcmp(key, "center_rod_activity") == 0)
  {
  	obj->center_rod_activity = atof(value);
  }
  else if(strcmp(key, "sphere_activity") == 0)
  {
  	obj->sphere_activity = atof(value);
  }
  else if(strcmp(key, "hot_voxel_activity") == 0)
  {
  	obj->hot_voxel_activity = atof(value);
  }
  else if(strcmp(key, "negative_voxel_activity") == 0)
  {
  	obj->negative_voxel_activity = atof(value);
  }
  else if(strcmp(key, "background_activity") == 0)
  {
  	obj->background_activity = atof(value);
  }
  else if(strcmp(key, "shell_attenuation") == 0)
  {
  	obj->shell_attenuation = atof(value);
  }
  else if(strcmp(key, "body_attenuation") == 0)
  {
  	obj->body_attenuation = atof(value);
  }
  else if(strcmp(key, "center_rod_attenuation") == 0)
  {
  	obj->center_rod_attenuation = atof(value);
  }
  else if(strcmp(key, "sphere_attenuation") == 0)
  {
  	obj->sphere_attenuation = atof(value);
  }
  else if(strcmp(key, "num_x_pixels") == 0)
  {
  	obj->num_x_pixels = atoi(value);
  }
  else if(strcmp(key, "num_y_pixels") == 0)
  {
  	obj->num_y_pixels = atoi(value);
  }
  else if(strcmp(key, "image_size_x") == 0)
  {
  	obj->image_size_x = atof(value);
  }
  else if(strcmp(key, "image_size_y") == 0)
  {
  	obj->image_size_y = atof(value);
  }
  else if(strcmp(key, "slice_thickness_z") == 0)
  {
  	obj->dz = atof(value);
  }
  else if(strcmp(key, "image_size_z") == 0)	//have users give total size, although program uses half size
  {
  	obj->image_size_half_z = (atof(value)/ 2.0);
  }
  else if(strcmp(key, "num_fine") == 0)
  {
  	obj->num_fine = atoi(value);
  }
  else if(strcmp(key, "show_hot_voxel") == 0)
  {
	int type = atoi(value);
	
	if(type == 1)	//0 is do not add hot voxel- default, 1 to add
	{	obj->b_ShowHotVoxel = true;
	}
  }  
  //--------------Noise/smoothing chararacteristics---------------
  else if(strcmp(key, "show_test_pattern") == 0)
  {
	int type = atoi(value);
	
	if(type != 0)	//0 is no noise (default), anything else is noise and possibly smoothing
	{	obj->b_show_test_pattern = true;
	}
  }
  else if(strcmp(key, "test_pattern_slice") == 0)
  {
	obj->test_pattern_slice = (atoi(value)-1);
  } 
  else if(strcmp(key, "test_pattern_volume_centerxy") == 0)
  {
	prefixes = (char *) strchr(value,',');
	char input[256];
	strcpy(input, prefixes+1);
	obj->test_pattern_volume_centery = atof(input);
	input[0] = '\0';
	strcpy(input, value);
	input[prefixes - value] = '\0';
	obj->test_pattern_volume_centerx = atof(input);
  }
  else if(strcmp(key, "test_pattern_extentxy") == 0)
  {
	obj->test_pattern_extentxy = atof(value);
  }
  else if(strcmp(key, "test_pattern_volume_extentz") == 0)
  {
	obj->test_pattern_volume_extentz = atof(value);
  }
  else if(strcmp(key, "test_pattern_area_centerxy") == 0)
  {
	prefixes = (char *) strchr(value,',');
	char input[256];
	strcpy(input, prefixes+1);
	obj->test_pattern_area_centery = atof(input);
	input[0] = '\0';
	strcpy(input, value);
	input[prefixes - value] = '\0';
	obj->test_pattern_area_centerx = atof(input);
  }
   //--------------Noise/smoothing chararacteristics---------------
  else if(strcmp(key, "CalculateNoise") == 0)
  {
	int type = atoi(value);
	
	if(type != 0)	//0 is no noise (default), anything else is noise and possibly smoothing
	{	obj->b_CalculateNoise = true;
	}
  }
  else if(strcmp(key, "AmountNoise") == 0)
  {
  	obj->AmountNoise = atof(value);
  }
  else if(strcmp(key, "GaussianSmooth") == 0)
  {
	int type = atoi(value);
	
	if(type == 0)	//0 is do not smooth , only valid with noise - default is to smooth
	{	obj->b_GaussianSmooth = false;
	}
  }
  else if(strcmp(key, "smooth_fwhm") == 0)
  {
  	obj->smooth_fwhm = atof(value);
  }
 /* else if(strcmp(key, "noisy_slice") == 0)
  {
	obj->noisy_slice = (atoi(value)-1);
  } */
  //--------------Run mode chararacteristics---------------
  else if(strcmp(key, "DRO_Type") == 0)
  {
	int type = atoi(value);
	
	if(type != 0)	//0 is emission, anything else is transmission
	{	//transmission image
		DRO_Type = DRO_TRANSMISSION;
	}
  }
  else if(strcmp(key, "OutputFileName") == 0)
  {
  	  if (strlen(value) >= 256) 
	  {
		printf(" ReadCommandParameterFile> !!! Error: key-value '%s' should contain less than 256 characters\n", key);
		exit(1);
	  }
	  
	  strcpy(OutputFileName, value);
  }
  else if(strcmp(key, "OutputFolderName") == 0)
  {
	  if (strlen(value) >= 256) 
	  {
		printf(" ReadCommandParameterFile> !!! Error: key-value '%s' should contain less than 256 characters\n", key);
		exit(1);
	  }
	  
	  strcpy(OutputFolderName, value);
  }
  else if(strcmp(key, "Rescale_Slope_Per_Slice") == 0)
  {
  	int type = atoi(value);
	
	if(type != 0)	//0 is same rescale slope for all slicess (default) - otherwise calculate for each individual slice
	{
		obj->Rescale_Slope_Per_Slice = true;
	}
  } 
  else if(strcmp(key, "Create_DICOMDIR") == 0)
  {
  	int type = atoi(value);
	
	if(type != 0)	//0 is same rescale slope for all slicess (default) - otherwise calculate for each individual slice
	{
		obj->b_Create_DICOMDIR = true;
	}
  } 
  else if(strcmp(key, "DICOMDIR_Filename") == 0)
  {
	  if (strlen(value) >= 256) 
	  {
		printf(" ReadCommandParameterFile> !!! Error: key-value '%s' should contain less than 256 characters\n", key);
		exit(1);
	  }
	  
	  strcpy(InFile_DICOMDIR_fname, value);
  }
  else if(strcmp(key, "Gaussian_blob_sphere5") == 0)
  {
  	int type = atoi(value);
	
	if(type != 0)	//1 is do the gaussian smearing....
	{
		obj->b_GaussianBlob_sphere5 = true;
	}
  } 
  else
  {
    printf(" PopulateCommandLineItemFromFile> Key %s not found.\n", key);
  	return false;	//not found
  }
  
  return true;
}
//-------------------------------------------------------------------------------------
bool DRO_Dicom_Object::ReadCommandParameterFile(char * filename)
{	
  FILE *f;
  char line[256], *c, * key, * value;
  line[0] = '\0';
  
  f = fopen(filename,"r");
  
  if(f == NULL) 
  {
      printf(" ReadParameterFile> File %s can not be opened.\n", filename);
	  exit(1);
  }
  
  DRO_Phantom_PET * DRO_phantom_temp = new DRO_Phantom_PET();	//we need a temp to store values prior to knowing what type of simulation this is 
  
  if(DRO_phantom_temp == NULL )
  {
	  printf("Error in allocation (3)! Aborting!\n");
	  exit(1);
  }
  
  while ((!feof(f))) 
  {
    c = (char *) fgets(line, 256, f);
	
	/*we parse looking for the equal sign, then our data is between that and the newline*/
	
	if(c != NULL)											  /*null on last read*/
	{
	  key = strtok (line,"=\n");
	  
	  if(key != NULL && key[0] != '#')	//look for a comment beginning at the start of line
	  {
		value = strtok (NULL," \n#");							  /*NULL continues reading from same string*/
		//may want to allow a comment field at end of line, say #
		
		if(value != NULL)
		{  if(PopulateCommandLineItemFromFile(DRO_phantom_temp, key, value) == false)
		  {
				ShowHelp();	
	  			printf("Error: Invalid parameter in command input file!\n");	 //failure to find and match key
				fclose(f);	
				exit(1);		 
		  }
		}
	  }
	}
  }
  
  fclose(f);	
  //allocate class variables and assign values
  AllocateClassItems();
  //copy the data in from the temp
  DRO_phantom_temp->CopyData(*DRO_phantom_obj, *DRO_phantom_temp);
  
  delete(DRO_phantom_temp);
  DRO_phantom_temp = NULL;
  
  //need to error check the values that have been set!
  if(!DRO_phantom_obj->AreInputValuesValid())
  {
	  printf("Error: Invalid input parameter!\n");
	  ShowParameters(false);
	  exit(1);
  }
  
  return true;
}
//-------------------------------------------------------------------------------------
char * DRO_Dicom_Object::GetNextLineYHDR(char * buf, int maxsize, FILE * fp)
{	/*a generic fgets so as to provide generic parsing, such as skipping over generic comments*/
	char * c = NULL;
	
	while(!feof(fp))  /*keep going till we get a valid read*/
	{  
	   c = (char *) fgets(buf, maxsize,  fp);
		
	   if(c == NULL)
	   {  return NULL;
	   }
	   else if(c[0] == ';')
	   {  /*do nothing, get the next line, this is a comment in YAFF*/
	   }
	   else
	   {
		  return buf;
	   }
	}
	return NULL;
}
//-------------------------------------------------------------------------------------
bool DRO_Dicom_Object::Read_YHDR_HeaderFile(InputParams * in_par, char * filename)
{
	FILE *fptr;
	fptr = fopen(filename,"r");
	if(fptr == NULL) 
	{
		printf(" Read_YHDR_HeaderFile> File %s can not be opened.\n", filename);
		exit(1);
	}

	/* use a function pointer to make this code generic...the only specifictiy is required for assigning values and header type*/
	char * line = (char *) malloc(256);	
	char  *c, * key, * value;
	line[0] = '\0';

	if( fseek(fptr, 0, 0) ) return( -1 );
	bool bInHeader = false;
	
	while (!feof(fptr)) 
	{
	  c = (char *) GetNextLineYHDR(line, 256, fptr);
	  
	  /*we parse looking for the equal sign, then our data is between that and the newline*/
	  
	  if(c != NULL)							  /*null on last read*/
	  {		
											  /*we stop when we get a MH_END - looking for some data in the subheader*/
		if(strcmp(line, SH_END) == 0)	
		{ return 0;							  /*getting to eof without a SH_END is an error*/
		}
	  
		if(bInHeader)
		{ 
		  key = strtok (line,"=\n");
		  value = strtok (NULL,"=\n");		  /*NULL continues reading from same string*/
		  	  
		   if(PopulateYHDRItemFromFile(in_par, key, value) == false)
		  {
				ShowHelp();	
	  			printf("Error: Invalid parameter in command input file!\n");	 //failure to find and match key
				fclose(fptr);	
				exit(1);		 
		  }
		}
											  /*we need to read till we get a MH_START*/
		if(strcmp(line, SH_START) == 0)	
		{  bInHeader = true;				  /*now we can start*/
		}
	  }
	}
	fclose(fptr);	

	return -1;								  /*at end of file, no end found*/
}
//-------------------------------------------------------------------------------------
bool DRO_Dicom_Object::PopulateYHDRItemFromFile(InputParams * in_par, char * key, char * value)
{ /*Command parameters that control type & variation of the run*/
  //bfe 1.4.12 - well just use 2 separate functions rather than overloading to maintain archive proof of correctness/testing

  //--------------Phantom chararacteristics---------------
  if(strcmp(key, "x_dimension") == 0)
  {
  	in_par->x_raw = atoi(value);
  } 
  else if(strcmp(key, "y_dimension") == 0)
  {
  	in_par->y_raw = atoi(value);
  }
  else if(strcmp(key, "z_dimension") == 0)
  {
  	in_par->z_raw = atoi(value);
  }
  return true;
}
//-------------------------------------------------------
bool DRO_Dicom_Object::File_Exists(char * filename)
{	//reads UID header and return prefix
	//also should verify is read only, else create an error

  FILE *f;
  f = fopen(filename,"r");
  
  if(f == NULL) 
  {
    return false;
  }
  
  fclose(f);
  return true;	
}




