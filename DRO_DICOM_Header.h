/*
 *  DRO_DICOM_Header.h
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/13/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */
 
#ifndef DRO_DICOM_Header_H
#define DRO_DICOM_Header_H

#include "DRO_Phantom.h" 

class DRO_DICOM_Header
{
public:

	DRO_DICOM_Header();
	//our DICOM header generating functions
	virtual void SetDICOMHeaderFields(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj, int slice, char * uid_fname, char * InFile_UID_Date_fname, DcmFileFormat * dcmfile, bool braw_data) = 0;	//set from code
	
	bool ReadHeaderFieldsFileAndSet(DcmDataset *dataset, char * filename, DcmFileFormat * dcmfile);									//set from file input
	
	bool Read_SitePrefix_UID_File(char *ret, char * filename);															//from the UID file - each lab needs its own unique UID
	
protected:

	//well derive the child functions for ones that have differing use - note we leave a number generic though may only be used by emission
	//and use virtuals for similar but different implementations (between emission and transmission)
	//try and differentiate based on type of info being loaded, though divisions could vary in interpretation
	
	void SetImageDataDcmInfo(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj);			//image relevant
	void SetSliceThicknessDcmInfo(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj);		//slice thickness
	
	virtual void SetPixelSpacingDcmInfo(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj) = 0;			//pixel spacing specific
	virtual void SetHeaderSliceInfo(DcmDataset *dataset, int slice, DRO_Phantom * DRO_phantom_obj) = 0;		//slice
	virtual void SetInstitutionUIDDcmInfo(DcmDataset *dataset, int slice, char * uid_fname, char * InFile_UID_Date_fname, DcmFileFormat * dcmfile, char *UID_prefix) = 0;	
			
	//for looking up DCMTK strings and returning a parsable format
	OFString FindAndGetSequencedItemOFString(DcmDataset *dataset, const DcmTag & seqTag, const DcmTagKey &tagKey, int item = 0);
	OFString FindAndGetItemOFString(DcmDataset *dataset, const DcmTagKey &tagKey);
	
	void GetDateCode(char *ret, char * InFile_UID_Date_fname);									//get a date code for UID encoding, read from file if user supplies a link file, else is unique
	
	char UID_Date_Code[64];		//use the same UID date code for each file - so store it out once found
	
	//Insert string cleanup to meet dicom standards
	OFCondition ReplaceOrInsertString(DcmDataset *dataset, DcmTagKey *dct, char * value, DcmFileFormat * dcmfile);
	void RemoveLeadingZeros(char * input);
	void RemoveTrailingSpaces(char * value);
	
private:

	bool GetTagFromString(char * str, char * value, DcmTagKey *dct);							//returns the Dicom tag for a given matched string
	bool PopulateHeaderItemFromFile(DcmDataset *dataset, char * key, char * value, DcmFileFormat * dcmfile);				//populate a header item, uses DCMTK header fields
	
	bool PopulateSequencedHeaderItemFromFile(DcmDataset *dataset, char * key, char * value);	//populates a single sequenced item, to item 0.
	
	bool PopulateUID_SitePrefix_FromFile(char *ret,  char * key, char * value);							//read and populate the UID field site prefix from file
	
	unsigned short HextoShort(char * input);	//parse a private tag into appropriate representation
	short hexvalue(char val);					//caonvert to the hex representation
	
	bool AllowedtoInsertField(char * key);		//is this field allowed to be inserted from the header file?
	
	bool LoadUID_Date_From_DICOM_File(char *ret, char * InFile_UID_Date_fname);	//load up the in-file portion of the UID - the date code
	
	bool PopulateSequenced_ItemGroup(DcmItem  *item, char * key, char * value);
};

#endif