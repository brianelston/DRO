/*
 *  Constants.h
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/8/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#ifndef Constants_h
#define Constants_h

#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>  
#include <time.h> 

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcddirif.h"
using namespace std;

#define MERSENNE_RAND										//define to use the mersenne twister random number generator, else defaults to C random() call

#define UID_DRO_PREFIX "1.3.6.1.4.1.150.2.1"				//Use for DRO - the prefix UID we have been assigned - 150 is the UW, 2 is IRL, 1 is DRO from IRL
#define UID_DRES_PREFIX "1.3.6.1.4.1.150.2.2"				//Used for DICOM raw pixel data encapuslation mode

#define SOP_INSTANCE_UID	"1"
#define STUDY_INSTANCE_UID	"2"								//the 'identifier' we associate with the series UID - field identifier
#define SERIES_INSTANCE_UID	"3"
#define FRAMEOF_REFERENCE_UID "4"

#define PET_UID_CODE	"2"									//UID's might containing a type identifier
#define CT_UID_CODE	"1"


#define UID_FILENAME		"DRO_UID.txt"						//where the users sub-range prefix is stored
#define	MIN_UID_SUB_RANGE	1									//lets make sure the sub-range conforms to our assigned sub-range - or at least some range
#define MAX_UID_SUB_RANGE	1000	

#define TRANSMISSION_BACKGROUND_VALUE (-1000)					//default transmission BK value

#define CHECKER_BOARD_LOW	.1									//checkerboard pattern high/low values
#define CHECKER_BOARD_HIGH	.9

#define MAX_NUM_FINE	20										//maximum num_fine sampling divisions allowed

#define APP_NAME "DRO"											//for display only

#define MAX_IMAGE_VALUE_12BIT 4096.0							//for PET emission
#define MAX_IMAGE_VALUE 32767.0									//for PET emission
#define MIN_IMAGE_VALUE -32768.0								//for PET emission
#define MAX_IMAGE_RANGE 65535.0									//for PET emission

#define EMISSION_FOLDERNAME_DEFAULT "DRO_emission_noise_free"	//default folder names for output
#define TRANSMISSION_FOLDERNAME_DEFAULT "DRO_transmission"

#define DICOMDIR_DEFAULT_FNAME "DICOMDIR"

//lets define the parsing deliminators, as they may need to change
#define PR_TAG_DL_GE ';'	//Private tag deliminator for parsing group,element and value,type
#define SEQ_TAG_DL_ITEML	'['
#define SEQ_TAG_DL_ITEMR	']'

#define HOTVOXEL_SLICE		39

/*HEADER - group deliminators for parsing YAFF files*/
#define MH_START  "MAINHEADER\n"				  /*note \n - so we dont have to parse it out for comparison tets*/
#define MH_END	  "ENDMAINHEADER\n"
#define SH_START  "SUBHEADER\n"
#define SH_END	  "ENDSUBHEADER\n"
#define	BRICKLIST_START "BRICKLIST\n"
#define	BRICKLIST_END "ENDBRICKLIST\n"

//-----------------------------ENUMS--------------------------------------------------

typedef enum DRO_Object_Type {DRO_EMISSION = 0, DRO_TRANSMISSION = 1};	//what type of run that is happening - dont add fields or will confuse header creation (without virtual support class)!
typedef enum DRO_Rawdata_Type {RAW_STIR = 0, RAW_YAFFIMAGE = 1};	//what type of run that is happening - dont add fields or will confuse header creation (without virtual support class)!

#endif