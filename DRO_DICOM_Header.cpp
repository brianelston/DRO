/*
 *  DRO_DICOM_Header.cpp
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/13/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#include "DRO_DICOM_Header.h"
#include "DRO_Dicom_Object.h"  
//--------------------------------------------------------
DRO_DICOM_Header::DRO_DICOM_Header()
{
	UID_Date_Code[0] = '\0';
}
//--------------------------------------------------------
short DRO_DICOM_Header::hexvalue(char val)
{
	if(isdigit(val))
	{	
		//int ret = atoi(&val);
		short ret = val - '0';
		return ret;	//then if valid must be a digit
	}
		
	switch(toupper(val))
	{
		case 'A':
			return 10;
		break;
		case 'B':
			return 11;
		break;
		case 'C':
			return 12;
		break;
		case 'D':
			return 13;
		break;
		case 'E':
			return 14;
		break;
		case 'F':
			return 15;
		break;
		default:
		break;
		
	}
	return 0;
}
//--------------------------------------------------------
unsigned short DRO_DICOM_Header::HextoShort(char * input)
{
	//	input as int representation of hex value
	//this input consists of only the numeric hex values - thats DICOM
	unsigned short ret = 0;
	int multiplier = 1;
	int len = strlen(input);
	
	for(int i = len-1; i >= 0; i--)
	{
		ret += multiplier * hexvalue(input[i]);
		multiplier = multiplier * 16;
	}
	
	return ret;
}
//--------------------------------------------------------
OFString DRO_DICOM_Header::FindAndGetSequencedItemOFString(DcmDataset *dataset, const DcmTag & seqTag, const DcmTagKey &tagKey, int item)
{	//get the string return value of an item in a sequence
	OFString retval;
	DcmItem  *retitem = new DcmItem();
	
	dataset->findAndGetSequenceItem(seqTag, retitem, 0);
	retitem->findAndGetOFString(tagKey, retval);
	
	return retval;
}
//--------------------------------------------------------
OFString DRO_DICOM_Header::FindAndGetItemOFString(DcmDataset *dataset, const DcmTagKey &tagKey)
{	//get the string value of a  item
	OFString retval;
	dataset->findAndGetOFString(tagKey, retval);
	
	return retval;
}
//--------------------------------------------------------
void DRO_DICOM_Header::SetImageDataDcmInfo(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj)
{	//image data layout
	
	char temp[256] = {'\0'};
	sprintf(temp, "%d", DRO_phantom_obj->Get_num_dx());
	dataset->putAndInsertString(DCM_Rows, temp);
	
	sprintf(temp, "%d", DRO_phantom_obj->Get_num_dy());
	dataset->putAndInsertString(DCM_Columns, temp);
}
//--------------------------------------------------------
void DRO_DICOM_Header::SetSliceThicknessDcmInfo(DcmDataset *dataset, DRO_Phantom * DRO_phantom_obj)
{
	char temp[256] = {'\0'};
	if(((int)DRO_phantom_obj->Get_dz()) == DRO_phantom_obj->Get_dz())
	{	//slice thickness as an int if appropriate
		sprintf(temp, "%d", (int) DRO_phantom_obj->Get_dz());
	}
	else
	{
		sprintf(temp, "%f", DRO_phantom_obj->Get_dz());
	}
	dataset->putAndInsertString(DCM_SliceThickness, temp);
}
//---------------------------------------------------
//-------------------------------------------------------
bool DRO_DICOM_Header::LoadUID_Date_From_DICOM_File(char *ret, char * InFile_UID_Date_fname)
{	//in order for a CT and PET image to be linked, the UID must be the same - since it includes a date code,
	//we retrieve, that is parse out the date code from 1 of the 5 choices, easyest to use the study instance
	if (access(InFile_UID_Date_fname, F_OK) == 0)
	{	//if file exists, use it for date time
		
		DcmFileFormat fileformat;
		
		OFCondition status = fileformat.loadFile(InFile_UID_Date_fname);
		DcmDataset * dataset = fileformat.getDataset();
		
		char * val;
		dataset->findAndGetString(DCM_StudyInstanceUID, (const char * &) val);
		//now get the last element of the entry...thats the date string
		char * pch;
 		pch = strrchr(val,'.');		//get the last dot (.)
		strcpy(ret, pch + 1);		//and we want the text after that dot
		
		return true;
	}
	return false;
}
//-------------------------------------------------------
void DRO_DICOM_Header::GetDateCode(char *ret, char * InFile_UID_Date_fname)
{
	//we want to use the same date code for all slices, so store it out once we have it, and yse that
	if(UID_Date_Code[0] != '\0')	//then its already been populated
	{
		strcpy(ret, UID_Date_Code);
		return;
	}
	else if(InFile_UID_Date_fname[0] != '\0')	//then load the date from a pre-existing dicom file - this is the fusion link option - 
	{	//if the user has passed in a link_to file, then we get a date code from that file, else we generate a new one
		//allows PET and CT study to be linked through study instance
		if(LoadUID_Date_From_DICOM_File(ret, InFile_UID_Date_fname))
		{  	strcpy(UID_Date_Code, ret);
		  	printf("Date code linked from file is :%s\n", UID_Date_Code);
		  	return;	//only if successfull
		}
		else
		{
			
		  printf("ERROR: Cannot retrieve date code from file :%s. Creating new date code instead!\n", UID_Date_Code);
		}
		
	}
	//generate a new date code yyyymmddhhmmss - year, month, day, hour, minute, second

    time_t     now;
    struct tm *ts;
 
    // Get the current time
    now = time(NULL);
 
    ts = localtime(&now);
    strftime(UID_Date_Code, sizeof(UID_Date_Code), "%Y%m%d%H%M%S", ts);	//format to yyyymmddhhmmss
	
	printf("New date code is :%s\n", UID_Date_Code);
	strcpy(ret, UID_Date_Code);
	//we only want to do this once, so we save it out after the first time in UID_Date_Code
}
//-------------------------------------------------------
bool DRO_DICOM_Header::PopulateUID_SitePrefix_FromFile(char *ret,  char * key, char * value)
{
	if(strcmp(key, "InstanceUID_prefix") == 0)
  	{	
		strcpy(ret, value);
  	}	
	else
	{
		return false;
	}
	
	return true;
}
//-------------------------------------------------------
bool DRO_DICOM_Header::Read_SitePrefix_UID_File(char *ret, char * filename)
{	//reads UID header file and return prefix
	//also should verify is read only, else create an error

  FILE *f;
  char line[256] = {'\0'}, *c = NULL, * key = NULL, * value = NULL;
  line[0] = '\0';
  bool bDone = false;
  f = fopen(filename,"r");
  
  if(f == NULL) 
  {
    printf(" Read_SitePrefix_UID_File> File %s can not be opened.\n", filename);
    return false;
  }
  
  //just get the data - we should fail if the file has been modified......
  //also allot for # comment - so we can add info to the file
  while ((!feof(f)) && !bDone) 
  {
    c = (char *) fgets(line, 256, f);
	
	/*we parse looking for the equal sign, then our data is between that and the newline*/
	
	if(c != NULL)											  /*null on last read*/
	{
	  key = strtok (line,"=\n");
	  
	  if(key != NULL && key[0] != '#')							//look for a # comment beginning at the start of line
	  {
		value = strtok (NULL,"\n#");							  /*NULL continues reading from same string*/
		//may want to allow a comment field at end of line, say #
		
		if(value != NULL)
		{  
			if(PopulateUID_SitePrefix_FromFile(ret, key, value) == false)
			{
				  printf("Could not parse key: %s, value %s.\n", key, value);
				  DRO_Dicom_Object::ShowHelp();
				  fclose(f);	
				  return false;										  //failure to find and match key; though just keep rolling
			}
			else
			{	bDone = true;	//we only look for one UID entry
			}
		}
	  }
	}
  }
  
  fclose(f);
  return true;	
}
//--------------------------------------------------------
//-------------------------------------------------------
bool DRO_DICOM_Header::ReadHeaderFieldsFileAndSet(DcmDataset *dataset, char * filename, DcmFileFormat * dcmfile)
{	//reads header inpput file and sets appropriate fields
  FILE *f;
  char line[1024] = {'\0'}, *c = NULL, * key = NULL, * value = NULL;
  line[0] = '\0';
  f = fopen(filename,"r");
  
  if(f == NULL) 
  {
    printf(" ReadHeaderFieldsFileAndSet> File %s can not be opened.\n", filename);
    return false;
  }
  
  while ((!feof(f))) 
  {
    c = (char *) fgets(line, 1024, f);
	
	/*we parse looking for the equal sign, then our data is between that and the newline*/
	
	if(c != NULL)											  /*null on last read*/
	{
	  key = strtok (line,"=\n");
	  
	  if(key != NULL && key[0] != '#')						//look for a # comment beginning at the start of line
	  {
		value = strtok (NULL,"\n#");							  /*NULL continues reading from same string*/
		//may want to allow a comment field at end of line, say #
		
		if(value != NULL)
		{  
			if(value[0] == '[')
			{	//then its a sequenced item
				PopulateSequencedHeaderItemFromFile(dataset, key, value);
			}
			else if(PopulateHeaderItemFromFile(dataset, key, value, dcmfile) == false)	//then a normal dicom tag
			{
				  printf("Could not parse key: %s, value %s.\n", key, value);
				  DRO_Dicom_Object::ShowHelp();
				  fclose(f);	
				  return false;										  //failure to find and match key; though just keep rolling
			}
		}
		else	//add an empty item
		{	//insertEmptyElement  
			if(PopulateHeaderItemFromFile(dataset, key, "", dcmfile) == false)
			{
				  printf("Could not parse key: %s, value %s.\n", key, value);
				  DRO_Dicom_Object::ShowHelp();
				  fclose(f);	
				  return false;										  //failure to find and match key; though just keep rolling
			}
		
		}
	  }
	}
  }

  fclose(f);	
  
  return true;
}
//-------------------------------------------------------------------------------------
bool DRO_DICOM_Header::PopulateHeaderItemFromFile(DcmDataset *dataset, char * key, char * value, DcmFileFormat * dcmfile)
{ /*sets a given key item in header with value as string, unless special cased*/

	//need to get the tag from the string
	DcmTagKey dct;
	if(GetTagFromString(key, value, &dct))
	{
		OFCondition status;
		//altenatively: may want to try and insert so VR tag can be found in dictionary, and only on failure inset as an EVR_LO
		//the dict already has the group/element fields
        //10.22.14 - allowed character counter reduced in 10.9 to 112 character for entry MAX!
		if(strcmp(key, "DRO_PrivateTag") == 0)
		{	//sets an 'unknown' field
			//note tags should have a private creator ' reservation tag'
			
			char       *prefixes = NULL;
			//prefixes = (char *) strchr(value,',');
			prefixes = (char *) strchr(value, PR_TAG_DL_GE);
			DcmVR evr("LO");	//defaults to long string
			
			if(prefixes > 0)//in this case the type may be optionally encoded in value
			{  
				char input[1024] = {'\0'}, type_code[128] = {'\0'};
				strcpy(type_code, prefixes+1);
				//this si the type code
				strcpy(input, value);
				input[prefixes - value] = '\0';
				strcpy(value, input);	//the value part
				
				if(strcmp(type_code, "LO") != 0)	//if given, set the type to what the user wants
				{
				  evr.setVR(type_code);
				}
			 }
		
			//first private tag must be of the (gggg, 10) format
			DcmTag tag(dct.getGroup(), dct.getElement(), evr);
			//register the tag in the dictionary
			
			//seems to support various types added as string
			RemoveTrailingSpaces(value);
			RemoveLeadingZeros(value);	//some values start with a zero.....or are all zero's
			  
			if(dataset->tagExists(tag))	
			{					
				DcmElement  *el;
				dataset->findAndGetElement(tag, el);
				status = el->putString(value);
			}
			else
			{
				status = dataset->putAndInsertString(tag, value);	//new item
			}
		
			//has to have a type, else DCMTK will not insert a private tag
			//status = dataset->putAndInsertString(tag, value);	//works ok
			//status = ReplaceOrInsertString(dataset, &tag, value, dcmfile);	//causes error/???
			
		}
		else
		{
			//might want to dissallow certain tags....from user setting
			//DCM_StudyInstanceUID, DCM_SeriesInstanceUID, DCM_FrameOfReferenceUID, DCM_SOPInstanceUID, DCM_MediaStorageSOPInstanceUID
			if(AllowedtoInsertField(key))
			{	
				if(value == NULL || value[0] == '\0')	//insert empty
				{
					status = dataset->insertEmptyElement(dct);	//empty
				}
				else	//has a value to set
				{
					status = ReplaceOrInsertString(dataset, &dct, value, dcmfile);
				}
			}
		}
		if (status.bad())
		{	cerr << "Error: cannot populate DICOM header field (" << status.text() << ")" << dct.getGroup() << " " << dct.getElement() << endl;
			return false;
		}
  		return true;
	}
  
  return false;
}
//-------------------------------------------------------------------------------------
void DRO_DICOM_Header::RemoveLeadingZeros(char * input)
{	//leading zeros in front of adecimal point not allowed in Dicom
	char newstr[1024]= {'\0'};
	int i = 0;
	int slen = strlen(input);
	bool bsuccess = false;

	//should also look for the decimal point
	for(i = 0; i < slen-1; i++)
	{
		if((input[i] != '0') || (input[i] == '.' && i > 0 && input[i-1] == '0')) 
		{	bsuccess = true;	
			break;
		}
	}
	
	if(i != 0 && bsuccess)
	{
		strncpy(newstr, &input[i], slen - i);
	//	newstr[slen - i] = '\0';
		strcpy(input, newstr);
	}
}//-------------------------------------------------------------------------------------
void DRO_DICOM_Header::RemoveTrailingSpaces(char * value)
{	//trailing spaces, tabs

	int slen = strlen(value);
	int i;
	for(i = slen-1; i >= 0; i--)
	{
		if(value[i] != ' ' && value[i] != '\t')		//look for the last character that is not a space or tab
		{
			break;
		}
	}
	if(i != slen-1)
	{
		value[i+1] = '\0';	//and set the end of string there
	}
}
//-------------------------------------------------------------------------------------
OFCondition DRO_DICOM_Header::ReplaceOrInsertString(DcmDataset *dataset, DcmTagKey *dct, char * value, DcmFileFormat * dcmfile)
{	//generic put string insert compartmentalization:
	//allows values to be dealt with in DICOM required formats or methods

	RemoveTrailingSpaces(value);
	
	//not needed if people enter values correctly, but better to error check and clean up! DICOM standard requires no leading zero's!
	RemoveLeadingZeros(value);	//some values start with a zero.....or are all zero's

	OFCondition status;
	if(dct->getGroup() == 0x0002)	//then is meta file info, group == 0002
	{
		DcmMetaInfo *metinf = dcmfile->getMetaInfo();
		delete metinf->remove(*dct); 
	
		if(metinf->tagExists(*dct))		//should always exist in meta header
		{					
			DcmElement  *el;
			metinf->findAndGetElement(*dct, el);
			status = el->putString(value);
		}
		else
		{	status = metinf->putAndInsertString(*dct, value);	//new item
		}
	}
	else	//normal, non-meta, tag
	{
	  if(dataset->tagExists(*dct))	
	  {					
		  DcmElement  *el;
		  dataset->findAndGetElement(*dct, el);
		  status = el->putString(value);
	  }
	  else
	  {
		  status = dataset->putAndInsertString(*dct, value);	//new item
	  }
	}
	return status;
}
//-------------------------------------------------------------------------------------
bool DRO_DICOM_Header::AllowedtoInsertField(char * key)
{
	//DCM_StudyID, DCM_Manufacturer, DCM_StudyDescription, DCM_SeriesDescription
	if((strcmp(key, "DCM_SOPInstanceUID") == 0) || (strcmp(key, "DCM_MediaStorageSOPInstanceUID") == 0) 
	|| (strcmp(key, "DCM_StudyInstanceUID") == 0) || (strcmp(key, "DCM_SeriesInstanceUID") == 0) || (strcmp(key, "DCM_FrameOfReferenceUID") == 0))
	//|| (strcmp(key, "DCM_StudyID") == 0) || (strcmp(key, "DCM_Manufacturer") == 0) || (strcmp(key, "DCM_StudyDescription") == 0)
	//|| (strcmp(key, "DCM_SeriesDescription") == 0))
  	{
		printf("ERROR!! User is not allowed to set (from header file): %s.\n", key);
		return false;
	}
	
	if((strcmp(key, "DCM_PixelSpacing") == 0) || (strcmp(key, "DCM_InstanceNumber") == 0) 
	|| (strcmp(key, "DCM_SliceLocation") == 0) || (strcmp(key, "DCM_ImagePositionPatient") == 0) || (strcmp(key, "DCM_SliceThickness") == 0)
	|| (strcmp(key, "DCM_Rows") == 0) || (strcmp(key, "DCM_Columns") == 0) || (strcmp(key, "DCM_ImageIndex") == 0) || (strcmp(key, "DCM_FrameReferenceTime") == 0))
  	{
		printf("WARNING!! User setting (from header file): %s. This could cause the DICOM image to be invalid, as these fields are directly related to image dimensions.\n", key);
		//though we let you set them if you want! but lets tell you.
		//return false;
	}
	//also DCMTK wont set DCM_Manufacturer, DCM_ImplementationClassUID - at least not within the meta-header

	return true;
}
//-------------------------------------------------------------------------------------
bool DRO_DICOM_Header::GetTagFromString(char * str, char * value, DcmTagKey *dct)
{	
	//every entry in the header file needs to be parsed
	
	if(strcmp(str, "DRO_PrivateTag") == 0)
  	{	//allow for private tags - let user set group/element explicitly
	
	  char *key2 = strtok (value,"=\n");
	  char* value2 = NULL;
	  
	  if(key2 != NULL && key2[0] != '#')							//look for a # comment beginning at the start of line
	  {	
	  	value2 = strtok (NULL,"\n#");							  /*NULL continues reading from same string*/
		
		//if(value2 != NULL)	//need to allow null for empty items private tags
		//{
			char       *prefixes = NULL;
			//prefixes = (char *) strchr(key2,',');
			prefixes = (char *) strchr(key2, PR_TAG_DL_GE);
			char input[256] = {'\0'};
			strcpy(input, prefixes+1);
			//input converts to hex format
		  	unsigned short element_tag = (unsigned short) HextoShort(input);
			input[0] = '\0';
			strcpy(input, key2);
			input[prefixes - key2] = '\0';
		  	unsigned short group_tag = (unsigned short) HextoShort(input);
		  
			dct->set(group_tag, element_tag);
			//and need to change value to remove group, element and reflect the standalone variable
		
			if(value2 != NULL)
			{	strcpy(value, value2);  
			}
			else
			{
				strcpy(value, "");
			}
			return true;
		//}
	  }
  	}
	else	//useing name, not group, element lookup
	{
		//use findTagFromName here
		DcmTag tag;
		char tagname[256] = {'\0'};
		strncpy(tagname, &str[4], strlen(str) - 4 );	//remove the 'DCM_' prefix
		tagname[strlen(str) - 4] = '\0';
		
		OFCondition status = DcmTag::findTagFromName(tagname, tag);	//i.e. PatientName or likewise
		
		if(status.good())
		{	*dct = tag;
			return true;
		}
	}
	
	return false;	//could not set!
}
//-------------------------------------------------------------------------------------
bool DRO_DICOM_Header::PopulateSequenced_ItemGroup(DcmItem  *item, char * key, char * value)
{
	char *b, *seq_key, *seq_value;
	char seq_kvpair[1024];
	DcmTagKey dct;
	OFCondition status;
	//1) strip off inner & outer ()
	
	char value_new[1024];
	strcpy(value_new, value);
	b = strtok (value_new,"[]\n");

	//get the first KV pair
	char * p_kv;
	p_kv=strchr(b,':');
	strncpy(seq_kvpair, b, strlen(b) - strlen(p_kv));
	seq_kvpair[strlen(b) - strlen(p_kv)] = '\0';
	seq_key = strtok (seq_kvpair,"=\n");
	
	while(seq_key != NULL)	//ADD ALL SEQUENCE KEYS
	{	
		seq_value = strtok (NULL,":\n");
		
		if(seq_value != NULL)	//and if seq_value[0] == '(', then we have a new sequence
		{
			if(GetTagFromString(seq_key, seq_value, &dct))
			{
				item->putAndInsertString(dct, seq_value);
			}
			else
			{	continue;
			}
		}
		//now get everything in b past the :, and reuse that for the next tag
		char * pch;
		pch=strchr(b,':');
		
		if(pch != NULL)
		{
			strcpy(b, &pch[1]);
			pch=strchr(b,':');	//and the next one
			if(pch != NULL)
			{
				strncpy(seq_kvpair, b, strlen(b) - strlen(pch));
				seq_kvpair[strlen(b) - strlen(pch)] = '\0';
		
				if(seq_kvpair != NULL)
				{	seq_key = strtok (seq_kvpair,"=\n");	//get the next one
				}
				else
				 {	seq_key = NULL;
					continue;
				 }
			  } 
			 else	//last one
			 {	if(b != NULL)
			 	{
					strcpy(seq_kvpair, b);
					seq_key = strtok (seq_kvpair,"=\n");	//get the next one
				}
			 	else
				{
					seq_key = NULL;
					continue;
				}
			 }
		 }
		 else
		 {	seq_key = NULL;
		 	continue;
		 }
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool DRO_DICOM_Header::PopulateSequencedHeaderItemFromFile(DcmDataset *dataset, char * key, char * value)
{ 	//sets a given sequenced item subset in header with value as string, unless special cased

	//format 1: single item sequence
	//DCM_AnatomicRegionSequence=(DCM_CodeValue=T-D0010:DCM_CodingSchemeDesignator=SRT:DCM_CodeMeaning=WHOLEBODY)
	//format 2: nested item sequence
	//DCM_PatientOrientationCodeSequence=(DCM_CodeValue=F-10450:DCM_CodingSchemeDesignator=99SDM:DCM_CodeMeaning=recumbent)DCM_PatientOrientationModifierCodeSequence=(DCM_CodeValue=F-10450:DCM_CodingSchemeDesignator=99SDM:DCM_CodeMeaning=supine)
	//bfe -  10.27.11 - requires correct input style - limited in robustness, and only accepts single sequences (ie insert at location 0) or single nested item sequences
	
	DcmItem  *item = new DcmItem();
	DcmItem  *item2 = new DcmItem();
	DcmItem  *retitem = new DcmItem();
	DcmTagKey dct;
	OFCondition status;
	
	if(GetTagFromString(key, value, &dct))
	{
		//if its a duplicate, then extract and change/modify existing value - we dont want 2 entries in the dicom file
		//required to allow iniitialization and file overwrites, which requires 2 reads of the header file
		status = dataset->findAndGetSequenceItem(dct, retitem, 0);
		if(status.good())
		{
			item = retitem;
		}
	}
	
	char value_new[1024];
	strcpy(value_new, value);
	
	//deal with possibility of a nested sequenced item
	char       *prefixes;
	int		i_nesteditems = 0;
	prefixes = (char *) strchr(value_new,']');
	char input2[1024], in1[1024];
	strcpy(input2, prefixes+1);
	
	strcpy(in1, value_new);
	in1[prefixes - value_new] = '\0';
	if(strlen(input2) == 0)
	{	//then only 1 item
		strcpy(value_new, value);
	}
	else
	{	//then we have a nested sequence
		strcpy(value_new, in1);
		i_nesteditems = 1;
	}
	
	//populate the first group
	PopulateSequenced_ItemGroup(item, key, value_new);
	//item->computeGroupLengthAndPadding(EGL_withGL, EPD_withPadding, EXS_LittleEndianExplicit, EET_ExplicitLength);
	item->calcElementLength( EXS_LittleEndianExplicit, EET_ExplicitLength);
	
	
	//and if we have a second nested group, populate it and insert
	if(i_nesteditems == 1)
	{	//secong group, need to strip out the new key
		char * key2, * value2;
		
		key2 = strtok (input2,"=\n");
	  
		if(key2 != NULL && key2[0] != '#')						//look for a # comment beginning at the start of line
		{
		  value2 = strtok (NULL,"\n#");							  /*NULL continues reading from same string*/
		  //may want to allow a comment field at end of line, say #
		  
		  if(value2 != NULL)
		  {  
		  	  //currently supports 2 nested items maximum
			  PopulateSequenced_ItemGroup(item2, key2, value2);
			  
			  if(GetTagFromString(key2, value2, &dct))
			  {
			  		status = item->findAndGetSequenceItem(dct, retitem, 0);
					if(!status.good())
					{
						//item2->computeGroupLengthAndPadding(EGL_withGL, EPD_withPadding, EXS_LittleEndianExplicit, EET_ExplicitLength);
						item2->calcElementLength( EXS_LittleEndianExplicit, EET_ExplicitLength);
				  		item->insertSequenceItem(dct, item2, 0);
					}
			  }
			}
		}
	}
	
	//and insert the item group in the dataset
	if(GetTagFromString(key, value, &dct))
	{
		//only insert if it does not exist, else get and change values - we dont want duplicates
		status = dataset->findAndGetSequenceItem(dct, retitem, 0);
		if(!status.good())
		{	
			dataset->insertSequenceItem(dct, item, 0);		//for now only supports level 0 item and 1 level nested items
		}

  		return true;
	}
  
  	return false;
}
