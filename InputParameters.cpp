/*
 *  InputParameters.cpp
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/25/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *  parses input parameters
 */

#include "InputParameters.h"
#include "Constants.h"
#include "DRO_Dicom_Object.h"

InputParams::InputParams()
{
	InFileCommands_fname[0] = '\0';
	InFileheaderFields_fname[0] = '\0';
	
	strcpy(InFileUID_fname, UID_FILENAME);	//as its required, lets preload the filenam, and not accept other names - for now
	
	InFolderPixelData[0] = '\0';
	InFilePixelData_fname[0] = '\0';
	
	bOutputDataBlock = false;
	
	InFile_UID_Date_fname[0] = '\0';
	
	b_RawDataInput = false;
	x_raw = 1;
	y_raw = 1;
	z_raw = 1;
	Bq_raw_scalefactor = 1.0;
}
//--------------------------------------------------------
void InputParams::ParseCommandLine(int argc, char *argv[])
{
  int         option;
  extern char *optarg;
  
  while ((option = getopt (argc, argv, "di:h:u:r:s:x:y:z:")) != EOF) 
  {
    switch (option) 
	{
	 case 'd' :	//raw data output? (default dicom stack only) - for testing
		   bOutputDataBlock = true;
		break;
	  case 'i' :	//input file for parameters
		   if (strlen(optarg) >= 256) 
		  {
			printf(" ParseCommandLine> !!! Error: input parameter '%s' should contain less than 256 characters\n", optarg);
			exit(1);
		  }
		  if (sscanf(optarg, "%s", InFileCommands_fname) != 1) 
		  {
			printf("error decoding -i %s\n", optarg);
			exit(1);
		  }
		break;
	  case 'h' :	//input file for custom header fields
		  if (strlen(optarg) >= 256) 
		  {
			printf(" ParseCommandLine> !!! Error: input parameter '%s' should contain less than 256 characters\n", optarg);
			exit(1);
		  }
		  if (sscanf(optarg, "%s", InFileheaderFields_fname) != 1) 
		  {
			printf("error decoding -h %s\n", optarg);
			exit(1);
		  }
		break;
	 /* case 'p' :	//input folder for pre-generated pixel data
		  if (strlen(optarg) >= 256) 
		  {
			printf(" ParseCommandLine> !!! Error: input parameter '%s' should contain less than 256 characters\n", optarg);
			exit(1);
		  }
		  if (sscanf(optarg, "%s", InFolderPixelData) != 1) 
		  {
			printf("error decoding -p %s\n", optarg);
			exit(1);
		  }
		break;
	  case 'q' :	//input file for pre-generated pixel data
		  if (strlen(optarg) >= 256) 
		  {
			printf(" ParseCommandLine> !!! Error: input parameter '%s' should contain less than 256 characters\n", optarg);
			exit(1);
		  }
		  if (sscanf(optarg, "%s", InFilePixelData_fname) != 1) 
		  {
			printf("error decoding -q %s\n", optarg);
			exit(1);
		  }
		break;*/
	  case 'r' :	//input file for pre-generated raw pixel data - just image bytes to be wrapped in a DICOm style header
		  if (strlen(optarg) >= 256) 
		  {
			printf(" ParseCommandLine> !!! Error: input parameter '%s' should contain less than 256 characters\n", optarg);
			exit(1);
		  }
		  if (sscanf(optarg, "%s", InFilePixelData_fname) != 1) 
		  {
			printf("error decoding -q %s\n", optarg);
			exit(1);
		  }
		  b_RawDataInput = true;
		  //assumed to be float 32 bit litte endian data (STIR) or unsigned 16 bit big endian (-I yaff)
		break;  
		case 's' :	  //raw data
		 if (sscanf(optarg, "%f", &Bq_raw_scalefactor) != 1) //may not neccesarilly be in bequerlys- but just assume is some number 
		  {
			printf("error decoding -s %s\n", optarg);
			exit(1);
		  }
		break;
	  case 'x' :	  //raw data
		 if (sscanf(optarg, "%d", &x_raw) != 1) 
		  {
			printf("error decoding -x %s\n", optarg);
			exit(1);
		  }
		break;
	  case 'y' :	  //raw data
		 if (sscanf(optarg, "%d", &y_raw) != 1) 
		  {
			printf("error decoding -y %s\n", optarg);
			exit(1);
		  }
		break;
	  case 'z' :	  //raw data
		 if (sscanf(optarg, "%d", &z_raw) != 1) 
		  {
			printf("error decoding -z %s\n", optarg);
			exit(1);
		  }
		break;
	  case 'u' :	//input file for getting date for UID - so we can link the PET and CT images...through the same study instance
		  if (strlen(optarg) >= 256) 
		  {
			printf(" ParseCommandLine> !!! Error: input parameter '%s' should contain less than 256 characters\n", optarg);
			exit(1);
		  }
		  if (sscanf(optarg, "%s", InFile_UID_Date_fname) != 1) 
		  {
			printf("error decoding -u %s\n", optarg);
			exit(1);
		  }
		break;
			default: 
			DRO_Dicom_Object::ShowHelp();
			exit(0);
	}
  }
 
 	//show user the parameters
  printf("Input filename for command parameters is : %s\n", InFileCommands_fname);
  printf("Input filename for header parameters is : %s\n", InFileheaderFields_fname);
  
  if(InFile_UID_Date_fname[0] != '\0')
  {		printf("Input filename for UID date code prefix is : %s\n", InFile_UID_Date_fname);
  }
   
  if(InFolderPixelData[0] != '\0')
  {	
		printf("Input folder for pre-existing DICOM pixel data is : %s\n", InFolderPixelData);
		printf("Input file for pre-existing DICOM pixel data is : %s\n", InFilePixelData_fname);
  }
  
  if(b_RawDataInput)
  {
		printf("Input folder for pre-existing RAW pixel data is : %s\n", InFolderPixelData);
		printf("Input file for pre-existing RAW pixel data is : %s\n", InFilePixelData_fname);
		printf("Size of RAW pixel data x: %d y: %d z: %d\n", x_raw, y_raw, z_raw);
		printf("Scaling of RAW pixel data is %f\n", Bq_raw_scalefactor);
  }
 
  if(bOutputDataBlock)
  {	
	  printf("Raw data image will be output.\n");
  }

  //need to error check the values that have been set! - no error checking on raw pixel data input params though!
  if(!AreInputValuesValid())
  {
	  printf("Error: Invalid input parameter!\n");
	  DRO_Dicom_Object::ShowHelp();
	  exit(1);
  }
}
//--------------------------------------------------------
bool InputParams::AreInputValuesValid()
{  
	//test for required parameters
  if(InFileCommands_fname[0] == '\0' || !(File_Exists(InFileCommands_fname)))
  {
	  printf("Error: Failure to provide a Command parameter input file (-i)!\n");
	  return false;
  }
  else if(InFileheaderFields_fname[0] == '\0' || !(File_Exists(InFileheaderFields_fname)))
  {
	  printf("Error: Failure to provide a header definition input file (-h)!\n");
	  return false;
  }
  else if(InFileUID_fname[0] == '\0' || !(File_Exists(InFileUID_fname)))
  {
	  printf("Error: Failure to provide a UID prefix input file named: %s!\n", InFileUID_fname);
	  return false;
  }
  //others are optional
  else if(InFile_UID_Date_fname[0] != '\0' && !(File_Exists(InFile_UID_Date_fname)))
  {
	  printf("Error: UID date code link file requested does not exist: %s !\n", InFile_UID_Date_fname);
	  return false;
  }

	return true;
}
//-------------------------------------------------------
bool InputParams::File_Exists(char * filename)
{	//verify is read only, else create an error

  FILE *f;
  f = fopen(filename,"r");
  
  if(f == NULL) 
  {
    return false;
  }
  
  fclose(f);
  return true;	
}