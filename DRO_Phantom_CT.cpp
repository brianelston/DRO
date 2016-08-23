/*
 *  DRO_phantom_CT.cpp
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/19/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#include "DRO_phantom_CT.h"

DRO_Phantom_CT::DRO_Phantom_CT()
{
	strcpy(OutputFileName, "DRO_phantom_CT_Image");	//for raw data output
}
//--------------------------------------------------------
void DRO_Phantom_CT::InitializeDRO_Object()
{
	for(int i = 0; i < num_dro; i++)
	{
		DRO_object[i] = (double) TRANSMISSION_BACKGROUND_VALUE;
	}
}
//--------------------------------------------------------
bool DRO_Phantom_CT::ConvertImageDataToOutputFormat()
{	//perform conversions appropriate to the output format	

	for(int i = 0; i < num_dro; i++)
	{
		DRO_object[i] = DRO_object[i] + (-TRANSMISSION_BACKGROUND_VALUE);//1000;//want min value
	}
	
	//we want 12 bit	- bitmask to enforce - no scaling, real values
	CovertImageto_12bit_int16();
	
	return true;
}
//--------------------------------------------------------
bool DRO_Phantom_CT::CovertImageto_12bit_int16()
{	//image is not scaled - just mask & transfer it 
	memset(DRO_object_Uint16, 0, (sizeof(short) * num_dro));	
	
	for(int i = 0; i < num_dro; i++)
	{
		DRO_object_Uint16[i] = (unsigned short)((unsigned short) DRO_object[i]  & 0x0FFF);	//use a bimask to assure proper bit zeroing
	}
	
	return true;
}
//--------------------------------------------------------
bool DRO_Phantom_CT::GetDICOMPixelData(DcmDataset *dataset, int slice)
{	//retrieve the pixel data in a DICOM file

    const Uint16 * pixelData = NULL;
	OFCondition status = dataset->findAndGetUint16Array(DCM_PixelData, pixelData);
	
	if (status.bad())
  	{	std::cerr << "Error: cannot get DICOM pixel data(" << status.text() << ") : slice = "<< slice << std::endl;
		return false;
	}
	memcpy(&DRO_object_Uint16[slice * num_dx * num_dy], pixelData, sizeof(short) * num_dx * num_dy);
	return true;
}
//--------------------------------------------------------
bool DRO_Phantom_CT::SetDICOMPixelData(DcmDataset *dataset, int slice)
{	//insert the pixel data in a DICOM file
	OFCondition status = dataset->putAndInsertUint16Array(DCM_PixelData,  &DRO_object_Uint16[slice * num_dx * num_dy], num_dx * num_dy);
	
	if (status.bad())
  	{	std::cerr << "Error: cannot insert DICOM pixel data(" << status.text() << ") : slice = "<< slice << std::endl;
		return false;
	}
	return true;
}
//--------------------------------------------------------
double DRO_Phantom_CT::CreateGeometryForSubsample(DRO_Sphere sphere[], int num_spheres, double x_fine_centers [], double y_fine_centers [], double z_fine_centers [])
{	//TRANSMISSION
	double mean = 0.0;
	double x;
	double y;
	double z;
	double val;
				
	double this_fine_voxel[num_fine][num_fine][num_fine];		//the fine sampleing

	//init
	for(int l = 0; l < num_fine; l++)
		for(int m = 0; m < num_fine; m++)
			for(int n = 0; n < num_fine; n++)
				this_fine_voxel[l][m][n] = (double) TRANSMISSION_BACKGROUND_VALUE;
	
	for(int xi = 0; xi < num_fine; xi++)	//loop over the fine voxels
	{
		for(int yi = 0; yi < num_fine; yi++)
		{
			for(int zi = 0; zi < num_fine; zi++)
			{
				x = x_fine_centers[xi];
				y = y_fine_centers[yi];
				z = z_fine_centers[zi];
					
				if(z >= -93.0 && z <= 93.0)	//skip for speed
				{
					//TRANSMISSION
					//loop the spheres
					//would be faster if excluded based on extreme z vals....
					bool bfoundval = false;
					if(z >= -52.0 && z <= -10.0)	//skip for speed - no spheres unless -52mm to -10 mm
					{
						for(int s = 0; s < num_spheres; s++)
						{
							val = sphere[s].GetDistanceValue(x, y, z);
										
							if(val <= sphere[s].Get_r()+1.0) 						//this_fine_voxel(sqrt((x-c(1)).^2+(y-c(2)).^2+(z+30).^2) <= r+1) = 0;
							{	this_fine_voxel[xi][yi][zi] = shell_attenuation;	
								bfoundval = true;
							}
							if(val <= sphere[s].Get_r())							//this_fine_voxel(sqrt((x-c(1)).^2+(y-c(2)).^2+(z+30).^2) <= r) = sphere_attenuation;
							{	this_fine_voxel[xi][yi][zi] = sphere_attenuation;
								bfoundval = true;
								break;
							}
							
						}
					}
					if(bfoundval)
						continue;
							
					//store out rather than calc twice
					double xsquared = pow(x, 2);
					double yplus35pow = pow(y+35, 2);
					
					if(z >= -90.0 && z <= 90.0)	
					{
						// The center rod:
						val = sqrt(xsquared + pow(y, 2));							//this_fine_voxel(sqrt(x.^2 + y.^2) <= 25 & z >= -90 & z <= 90) = center_rod_attenuation;
						//bfe - 9.21.11 - this should be < 24...shell should be inside
						if(val <= 25)	//if(val <= 26)	//25+1
						{	this_fine_voxel[xi][yi][zi] = shell_attenuation;
							bfoundval = true;					
						}
						if(val <= 23)	//25-2	//if(val <= 25)
						{	this_fine_voxel[xi][yi][zi] = center_rod_attenuation;
							continue;
						}
					}
					if(bfoundval)	continue;
						
					//TRANSMISSION	
					//faster if vals calculated exclusively, rather than for all, once found quit the loop, use precedence rather than overwriting
					// The lower rectangle:
					if(-70 <= x && x <= 70 && -115 <= y && y <= -35)				//this_fine_voxel(-70 <= x & x <= 70 & -112 <= y & y <= -35 & z >= -90 & z <= 90) = body_attenuation;
					{	this_fine_voxel[xi][yi][zi] = shell_attenuation;
						bfoundval = true;					
					}
					if(-70 <= x && x <= 70 && -112 <= y && y <= -35 && z >= -90.0 && z <= 90.0)		
					{	this_fine_voxel[xi][yi][zi] = body_attenuation;
						continue;
					}
					if(bfoundval)	continue;
						
					//The upper body:
					val = sqrt(xsquared + yplus35pow);								//this_fine_voxel(sqrt(x.^2+(y+35).^2) <= 147 & y >=-35 & z >= -90 & z <= 90) = body_attenuation;
					if(val <= 150 && y >= -35)	//147 + 3 - the shell
					{	this_fine_voxel[xi][yi][zi] = shell_attenuation;
						bfoundval = true;					
					}
					if(val <= 147 && y >= -35 && z >= -90.0 && z <= 90.0)			//the geometric object
					{	this_fine_voxel[xi][yi][zi] = body_attenuation;
						continue;
					}
					if(bfoundval)	continue;
					
					//Two lower half-circles:
					val = sqrt(pow(x+70, 2) + yplus35pow);							//this_fine_voxel(sqrt((x+70).^2 + (y+35).^2) <= 77 & y <= -35 & z >= -90 & z <= 90) = body_attenuation;
					if(val <= 80 && y < -35)	//77+3
					{	this_fine_voxel[xi][yi][zi] = shell_attenuation;
						bfoundval = true;					
					}
					if(val <= 77 && y < -35 && z >= -90.0 && z <= 90.0)
					{	this_fine_voxel[xi][yi][zi] = body_attenuation;
						continue;
					}
					if(bfoundval)	continue;
					
					
					val = sqrt(pow(x-70, 2) + yplus35pow);							//this_fine_voxel(sqrt((x-70).^2 + (y+35).^2) <= 77 & y <= -35 & z >= -90 & z <= 90) = body_attenuation;
					if(val <= 80 && y < -35)	//77+3
					{	this_fine_voxel[xi][yi][zi] = shell_attenuation;
						//bfoundval = true;					
					}
					if(val <= 77 && y < -35 && z >= -90.0 && z <= 90.0)
					{	this_fine_voxel[xi][yi][zi] = body_attenuation;
						continue;
					}
				}
			}
		}
	}			
	//TRANSMISSION
	mean = GetMean((double *)this_fine_voxel, (double) num_fine*num_fine*num_fine);
			
	return mean;
}
//--------------------------------------------------------
void DRO_Phantom_CT::FindMinMax_Sphere5(DRO_Sphere sphere[], int num_spheres, int this_x_center_index, int this_y_center_index, int this_z_center_index)
{ //do nothing for CT here
}
//--------------------------------------------------------
double DRO_Phantom_CT::Resample_Sphere5(DRO_Sphere sphere[], int num_spheres, double x_fine_centers [], double y_fine_centers [], double z_fine_centers [], int this_x_center_index, int this_y_center_index, int this_z_center_index, bool & bchanged)
{	//do nothing for CT here
	return 0;
}
//--------------------------------------------------------
bool DRO_Phantom_CT::WriteDROfile()
{
	//testing only - write out raw data
	FILE* fdes = NULL;
	if ((fdes = fopen(OutputFileName, "w")) == NULL)
	{
		printf("DRO_phantom::WriteDROfile: Error opening %s for writing!\n", OutputFileName);
		//this->~DRO_Phantom();	//dealloc through destructor
		exit(1);
	}
	
	fwrite(DRO_object_Uint16, sizeof(short), num_dro, fdes);		//unsigned short data
	fclose(fdes);	

	return true;
}
//--------------------------------------------------------
void DRO_Phantom_CT::Calculate_Max_Float()
{
	//does nothing, just allows overloads to compile
}
//--------------------------------------------------------
double DRO_Phantom_CT::Calculate_Max_Float_For_Slice(int slice)
{
	//does nothing, just allows overloads to compile
	return 0.0;
}
//--------------------------------------------------------
bool DRO_Phantom_CT::AreParametersWithinLimits()
{
	return true;
}
//--------------------------------------------------------
bool DRO_Phantom_CT::AreInputValuesValid()
{	//check the input values for allowable values

	const double min_attenuation = -1000.0;
	const double max_attenuation = 3095.0;

	if(	shell_attenuation < min_attenuation || shell_attenuation > max_attenuation ||
		body_attenuation < min_attenuation || body_attenuation > max_attenuation ||
		center_rod_attenuation < min_attenuation || center_rod_attenuation > max_attenuation ||
		sphere_attenuation < min_attenuation || sphere_attenuation > max_attenuation)
	{
		printf(" AreInputValuesValid> !!! Error: Attenuation values must be between %lf and %lf\n", min_attenuation, max_attenuation);
		return false;
	}	

	return DRO_Phantom::AreInputValuesValid();
}