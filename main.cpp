/*
 *  DRO_Phantom project - NEMA Digital Reference Object (DRO)
 *  DRO_phantom
 *
 *  Created by Brian Elston on 6/22/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 
 Generate a digital representation of the NEMA phantom. Create emission (PET) and transmission (CT) images, with a checkboard test pattern
 emission image as alternative. Control over scanner parameters including x,y,z size, pixel density, and intensity (emission) or density (transmission) values
 for the objects that comprise the individidual units of the NEMA phantom.
 
 Control of objects via the DRO_Dicom_Object class, which is composed of a pixel object and a header object, that create the DICOM stack object.
 
 Phantom and header have a derived child architecture to allow for common & unique functionality between the emission (EM) and transmission, that is attenuation, (ATT)
 instantiations. By overloading, source code is simplified and minimized as the emission and transmission objects share many similarities. In a few cases, as the CT image has fewer
 operations required, some overloads may be dummy overloads (empty).
 
 To compile the DCMTK libs this project needs to be linked against the zlib, DCMTK oflog, DCMTK ofstd, & DCMTK dcmdata libraries. (i.e.  -lz -loflog -lofstd -ldcmdata). DCMTK and zlib
 must be installed on the machine.
 
 This project is part of a QIBA RSNA research study investigating the display characteristics of various PET/CT scanner workstations.
 
 End user assumes all responsibility for use and implementation of this source code, or any product associated with or derived from this software.
 
 Based on algorithmic development by Larry Pierce and Paul Kinahan, UWMC, 2011.
 
 Raw data pixel encapsulation mode added to allow -I YAFF and .v STIR image files to be encapsulated in a DICOM header - to facilitate clients ability to inspect images more easily.
    
 *
 *	6.30.11 	bfe		Create pixel value geometry for NEMA phantom.
 *  7.7.11 		bfe 	Integrate DCMTK and intial testing of DCMTK & DcmFileFormat.
 *  9.26.11 	bfe 	Checkerboard test pattern added. File based UID.
 *  1.03.13 	bfe 	Added raw pixel data DICOM encapsulation mode.
 */

#include "InputParameters.h"
#include "DRO_Dicom_Object.h"

int main (int argc, char * argv[]) 
{
	time_t startTime;	//calc time for a run
	time_t endTime;
	
	startTime = time(NULL);
	
	InputParams Input_Vars;					//store the input
	DRO_Dicom_Object DRO_Dicom_obj;			//the object to operate on
	
	//read command line to get input file name & header input
	Input_Vars.ParseCommandLine(argc, argv);										//read the command line
	
	DRO_Dicom_obj.ReadCommandParameterFile(Input_Vars.InFileCommands_fname);		//input parameters via file, also error check
	
	if(!Input_Vars.b_RawDataInput)	//if DRO show now - are valid and takes a long time to run
	{  DRO_Dicom_obj.ShowParameters(Input_Vars.b_RawDataInput);													//show whats happening
	}
	
	DRO_Dicom_obj.GenerateDICOMStack(&Input_Vars);				//gnerate the dicom image & header stack
	
	if(Input_Vars.b_RawDataInput)
	{  DRO_Dicom_obj.ShowParameters(Input_Vars.b_RawDataInput);	 //we show after the parameters have been loaded - not valid beforehand
	}
	
	endTime = time(NULL);
	printf("Processing Time: %d\n", int(endTime - startTime));

    return 0;
}


