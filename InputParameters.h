/*
 *  InputParameters.h
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/25/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#ifndef InputParameters_H
#define InputParameters_H

class InputParams
{
public:

	InputParams();

	void ParseCommandLine(int argc, char *argv[]);

	//command line input parameter storage
	char InFileCommands_fname[256];				//command parameter file
	char InFileheaderFields_fname[256];			//header definition file
	
	char InFileUID_fname[256];					//UID prefix file
	
	char InFolderPixelData[256];				//optional input of DICOM stack - for pixel data
	char InFilePixelData_fname[256];			//pixel data filename prefix (to 000000.dcm convention)
	
	char InFile_UID_Date_fname[256];			//if the user specifies a date UID link file, we take the date from there	
	
	bool bOutputDataBlock;						//do we output the raw data file for testing?
	
	//bfe 1.2.13 - DICOM data encapsulation mode variables
	bool b_RawDataInput;	//is it raw data?
	int x_raw;				//raw data dimensions
	int y_raw;
	int z_raw;
	float Bq_raw_scalefactor; //scaling for raw data values as read in
	
private:

	bool AreInputValuesValid();
	bool File_Exists(char * filename);
};


#endif