/*
 *  DRO_Phantom_PET.cpp
 *  DRO_phantom
 *
 *  Created by Brian Elston on 7/19/11.
 *  Copyright 2011 University of Washington IRL. UWMC. All rights reserved.
 *
 */

#include "DRO_Phantom_PET.h"

DRO_Phantom_PET::DRO_Phantom_PET()
{
	strcpy(OutputFileName, "DRO_phantom_PET_Image");	//for raw data output
}
//--------------------------------------------------------
void DRO_Phantom_PET::InitializeDRO_Object()
{
	//if user wants to init with a double, then we need to do that manually
	for(int i = 0; i < num_dro; i++)
	{
		DRO_object[i] = background_activity;
	}
}
//--------------------------------------------------------
bool DRO_Phantom_PET::GetDICOMPixelData(DcmDataset *dataset, int slice)
{	//retrieve the pixel data in a DICOM file
	//Sint16Array wont work - we need to cast then all is fine
	
    const Uint16 * pixelData = NULL;
	OFCondition status = dataset->findAndGetUint16Array(DCM_PixelData, pixelData);
	
	if (status.bad())
  	{	std::cerr << "Error: cannot get DICOM pixel data(" << status.text() << ") : slice = "<< slice << std::endl;
		return false;
	}
	
	//cast as signed short
	memcpy(&DRO_object_int16[slice * num_dx * num_dy], (Sint16 *) pixelData, sizeof(short) * num_dx * num_dy);	//nope, wont cast
	
	return true;
}
//--------------------------------------------------------
bool DRO_Phantom_PET::SetDICOMPixelData(DcmDataset *dataset, int slice)
{	//insert the pixel data in a DICOM file
	//dont use putAndInsertSint16Array...- we need to cast then all is fine
	OFCondition status = dataset->putAndInsertUint16Array(DCM_PixelData,  (Uint16 *) &DRO_object_int16[slice * num_dx * num_dy], num_dx * num_dy);
	
	if (status.bad())
  	{	std::cerr << "Error: cannot insert DICOM pixel data(" << status.text() << ") : slice = "<< slice << std::endl;
		return false;
	}
	return true;
}
//--------------------------------------------------------
double DRO_Phantom_PET::CreateGeometryForSubsample(DRO_Sphere sphere[], int num_spheres, double x_fine_centers [], double y_fine_centers [], double z_fine_centers [])
{	//EMISSION
	double mean = 0.0;
	double x;
	double y;
	double z;
	double val;
				
	double this_fine_voxel[num_fine][num_fine][num_fine];		//the fine sampleing

	for(int l = 0; l < num_fine; l++)
		for(int m = 0; m < num_fine; m++)
			for(int n = 0; n < num_fine; n++)
				this_fine_voxel[l][m][n] = (double) background_activity;
		
	for(int xi = 0; xi < num_fine; xi++)	//loop over the fine voxels
	{
		for(int yi = 0; yi < num_fine; yi++)
		{
			for(int zi = 0; zi < num_fine; zi++)
			{
				//EMISSION
				x = x_fine_centers[xi];
				y = y_fine_centers[yi];
				z = z_fine_centers[zi];
					
				if(z >= -90.0 && z <= 90.0)	//skip for speed
				{
					//note: to optimize, dont test the spheres at extreme z, outside slice 29-52 (-50mm to -10mm)- should calculate max/min slice of largest diam sphere
					//loop the spheres	
					bool bfoundval = false;
					if(z >= -52.0 && z <= -10.0)	//skip for speed
					{
						for(int s = 0; s < num_spheres; s++)
						{
							val = sphere[s].GetDistanceValue(x, y, z);	
							
							if(b_GaussianBlob_sphere5 && (s == num_spheres-2))
							{	//bfe 3.21.12 - 2nd to largest we have a different algorithm - sphere 5 - gaussian blob - from lp for SUV Peak
								if(val <= sphere[s].Get_r())						
								{	
									this_fine_voxel[xi][yi][zi] = (exp((-8.0/pow(sphere[s].Get_r(),2))*pow(val,2)));  //bfe 10.19.12 - how did this get left here? test removal
									this_fine_voxel[xi][yi][zi] = (exp((-8.0/pow(sphere[s].Get_r(),2))*pow(val,2))) + body_activity;
									
									bfoundval = true;
									break;
								}
							}
							else  //do the normal sphere
							{				
							  if(val <= sphere[s].Get_r()+1.0) 						//this_fine_voxel(sqrt((x-c(1)).^2+(y-c(2)).^2+(z+30).^2) <= r+1) = 0;
							  {	this_fine_voxel[xi][yi][zi] = 0.0;					//quicker to set than test..redundant to the < r case, but lets leave for accuracy
								  bfoundval = true;
							  }
							  if(val <= sphere[s].Get_r())							//this_fine_voxel(sqrt((x-c(1)).^2+(y-c(2)).^2+(z+30).^2) <= r) = sphere_activity;
							  {	this_fine_voxel[xi][yi][zi] = sphere_activity;
								  bfoundval = true;
								  break;
							  }
							}
						}
					}
					if(bfoundval)
						continue;
						
					//EMISSION	
					//store out rather than calc twice
					double xsquared = pow(x, 2);
					double yplus35pow = pow(y+35, 2);
					
					// The center rod:
					val = sqrt(xsquared + pow(y, 2));								//this_fine_voxel(sqrt(x.^2 + y.^2) <= 25 & z >= -90 & z <= 90) = center_rod_activity;
					if(val <= 25)
					{	this_fine_voxel[xi][yi][zi] = center_rod_activity;
						continue;
					}
							
					//faster if vals calculated exclusively, rather than for all, once found quit the loop, use precedence rather than overwriting
					// The lower rectangle:
					if(-70 <= x && x <= 70 && -112 <= y && y <= -35)				//this_fine_voxel(-70 <= x & x <= 70 & -112 <= y & y <= -35 & z >= -90 & z <= 90) = body_activity;
					{	this_fine_voxel[xi][yi][zi] = body_activity;
						continue;
					}
						
					//The upper body:
					val = sqrt(xsquared + yplus35pow);								//this_fine_voxel(sqrt(x.^2+(y+35).^2) <= 147 & y >=-35 & z >= -90 & z <= 90) = body_activity;
					if(val <= 147 && y >= -35)
					{	this_fine_voxel[xi][yi][zi] = body_activity;
						continue;
					}
					
					//Two lower half-circles:
					val = sqrt(pow(x+70, 2) + yplus35pow);							//this_fine_voxel(sqrt((x+70).^2 + (y+35).^2) <= 77 & y <= -35 & z >= -90 & z <= 90) = body_activity;
					if(val <= 77 && y < -35)
					{	this_fine_voxel[xi][yi][zi] = body_activity;
						continue;
					}
					val = sqrt(pow(x-70, 2) + yplus35pow);							//this_fine_voxel(sqrt((x-70).^2 + (y+35).^2) <= 77 & y <= -35 & z >= -90 & z <= 90) = body_activity;
					if(val <= 77 && y < -35)
					{	this_fine_voxel[xi][yi][zi] = body_activity;
						continue;
					}
				}
			}
		}
	}			
	//EMISSION
	mean = GetMean((double *)this_fine_voxel, (double) num_fine*num_fine*num_fine);

	return mean;
}
//--------------------------------------------------------
void DRO_Phantom_PET::FindMinMax_Sphere5(DRO_Sphere sphere[], int num_spheres, int this_x_center_index, int this_y_center_index, int this_z_center_index)
{	
	//have to calculate min and max after the fact
	double val = 0;
	int sphere_ind = num_spheres-2;
	
	double x;
	double y;
	double z;
	
	x = x_center_vals[this_x_center_index];
	y = y_center_vals[this_y_center_index];
	z = z_center_vals[this_z_center_index];
	
	if(z >= -52.0 && z <= -10.0)	//skip for speed
	{
	  val = sphere[sphere_ind].GetDistanceValue(x, y, z);	
	  
	  if(val <= sphere[sphere_ind].Get_r())							
	  {	
		  double mean = DRO_object[this_x_center_index + (this_y_center_index * num_dx) + (this_z_center_index * num_dx * num_dy)];
	  
		  if(mean > max_sphere5)
		  {	max_sphere5 = mean;
		  }
		  if(mean < min_sphere5)
		  {	min_sphere5 = mean;
		  }
	  }
	 }
}
//--------------------------------------------------------
double DRO_Phantom_PET::Resample_Sphere5(DRO_Sphere sphere[], int num_spheres, double x_fine_centers [], double y_fine_centers [], double z_fine_centers [], int this_x_center_index, int this_y_center_index, int this_z_center_index, bool & bchanged)
{	//in PET resample sphere 5 - have to do after fact

	double val = 0;
	int sphere_ind = num_spheres-2;
	
	double x;
	double y;
	double z;
	double mean = 0.0;

	x = x_center_vals[this_x_center_index];
	y = y_center_vals[this_y_center_index];
	z = z_center_vals[this_z_center_index];
		
	if(z >= -52.0 && z <= -10.0)	//skip for speed
	{
	  val = sphere[sphere_ind].GetDistanceValue(x, y, z);	
	  //if(val <= sphere[sphere_ind].Get_r())			
	  if(val <= sphere[sphere_ind].Get_r())							
	  {	
		  bchanged = true;	//return the flag
		  
		  double old_vox_value = DRO_object[this_x_center_index + (this_y_center_index * num_dx) + (this_z_center_index * num_dx * num_dy)];
		  //calculate new value
		  mean = ((sphere_activity - body_activity) * (old_vox_value - min_sphere5) / (max_sphere5 - min_sphere5)) + body_activity;
	  }
	}
	
	return mean;

}
//--------------------------------------------------------
bool DRO_Phantom_PET::ConvertImageDataToOutputFormat()
{	//perform conversions appropriate to the output format	
	
	//do the unique data aspects here, as transmission does not have
	if(b_ShowHotVoxel)
	{	//have to set here as well, else scaling value is messed up on hot voxel slice
		int slicez = HOTVOXEL_SLICE;	//this needs to be adjusted based on the number of slices if we want it to appear in the same position relative (for runs with various # slices)
		slicez = (int) (num_dz * (slicez/109.0));	//might want to set directly to slice closest to -30mm
		
		SetDROPixelValue(128, 76, slicez, hot_voxel_activity );			//hot voxel
		SetDROPixelValue(128, 180, slicez, negative_voxel_activity );	//negative voxel
	}
	
	if(b_show_test_pattern && test_pattern_slice >= 0)			//do test pattern
	{
		AddCheckerBoardPatterns();
	}
	
	if(b_CalculateNoise)//new 3.26.12
	{
		AddNoise_double(AmountNoise, DRO_object);
	}

	ScaleImage();				//scaling, 0-1.0, finds and sets max, make all values positive if needed 
	
	CovertImageto_int16();		//convert to 2 -byte  (9.16.11: unsigned now-)
	
	if(b_CalculateNoise)// && noisy_slice >= 0)
	{	//we need to do this first on the real data.....else we smooth test voxels and test patteerns
		AddNoiseAndSmooth();//now just smooths
	}
	
	if(b_ShowHotVoxel && b_CalculateNoise)
	{	SetHotPixelValue();			//bfe 8.18.11 - we add this after the noise and smoothing - directly to the output buffer - lets keep it simple
		SetNegativePixelValue();
	}
	
	return true;
}
//--------------------------------------------------------
void DRO_Phantom_PET::SetHotPixelValue()
{	
	//test_pattern_volume_centerx / Get_dx()
	//int locz = 78;	//-12	//in mm
	int slicez = HOTVOXEL_SLICE;	//this needs to be adjusted based on the number of slices if we want it to appear in the same position relative (for runs with various # slices)
	slicez = (int) (num_dz * (slicez/109.0));	//might want to set directly to slice closest to -30mm
	
	printf("DRO_Phantom_PET::SetHotPixelValue: set at slice %d.\n", (slicez+1));
	
	SetDROIntValue_SHORT(128, 76, slicez, hot_voxel_activity );
	
//	SetDROPixelValue(128, 76, slicez, hot_voxel_activity );
/*	double d_activity = 0;
	if(Rescale_Slope_Per_Slice)
	{
		//SetDROPixelValue(128, 76, slicez, hot_voxel_activity );	//hot_voxel_activity/max_float_per_slice[slicez]
		d_activity = hot_voxel_activity/max_float_per_slice[slicez];
	}
	else
	{
		d_activity = hot_voxel_activity/max_float;
	}
	
	short s16_activity;

	if(d_16_activity >= 0)
	{	s16_activity = (short)(d_16_activity * MAX_IMAGE_VALUE);
		
	}
	else	//< =0...allot for slight change in scale length
	{	s16_activity = (short)(- d_16_activity * MIN_IMAGE_VALUE);
	}
	
	DRO_object_int16[x + (y * num_dx) + (z * num_dx * num_dy)] = s16_activity;*/
	
}
//--------------------------------------------------------
void DRO_Phantom_PET::SetNegativePixelValue()
{	
	int slicez = HOTVOXEL_SLICE;	//this needs to be adjusted based on the number of slices if we want it to appear in the same position relative (for runs with various # slices)
	slicez = (int) (num_dz * (slicez/109.0));	//might want to set directly to slice closest to -30mm

	printf("DRO_Phantom_PET::SetNegativePixelValue: set at slice %d.\n", (slicez+1));
	
	//SetDROPixelValue(128, 180, slicez, negative_voxel_activity );
	SetDROIntValue_SHORT(128, 180, slicez, negative_voxel_activity );
}
//--------------------------------------------------------
void DRO_Phantom_PET::Calculate_Max_Float()
{	//just determine from input file, not from DICOM, as is fairly complex - only done when we load a pre-existing file - 
	//if its new run - then we scan the image!
	
	max_float  = MAX(MAX(MAX(MAX(center_rod_activity, sphere_activity), background_activity), body_activity), hot_voxel_activity);		//max of activity in image
}
//--------------------------------------------------------
bool DRO_Phantom_PET::ScaleImage()
{	//1) get the max, 2) divide each object-pixel by max
	//max required to calculate rescale_slope
	//double max_val_per_slice[num_dz];
	
	if(!Rescale_Slope_Per_Slice)
	{
		max_float = 0.0;	//allow it to be reset
		for(int i = 0; i < num_dro; i++)
		{
			if(DRO_object[i] > max_float)
			{
				max_float = DRO_object[i];
				//would take larger of max or abs(min), but we know max is larger as is....
			}
		}
		
		for(int i = 0; i < num_dro; i++)
		{
			if(DRO_object[i] != 0.0)
			{
				DRO_object[i] = DRO_object[i]/max_float;
			}
		}
		
		if (max_float == 0.0)
		{	printf("DRO_phantom::ScaleImage: maximum is 0.0!\n");
			return false;
		}
	}
	else
	{	///we rescale by slice
	
		max_float = 0.0;	//allow it to be reset
		
		for(int j = 0; j < num_dz; 	j++)		
		{
			double this_max_float = 0.0;	//might want to store these out for calculating rescale slope
		
			for(int i = 0; i < num_dx * num_dy; i++)
			{
				int index = (j * num_dx * num_dy) + i;
				if(DRO_object[index] > this_max_float)
				{
					this_max_float = DRO_object[index];
					//would take larger of max or abs(min), but we know max is larger as is....
				}
			}
			
			for(int i = 0; i < num_dx * num_dy; i++)
			{
				int index = (j * num_dx * num_dy) + i;
				if(DRO_object[index] != 0.0)
				{
					DRO_object[index] = DRO_object[index]/this_max_float;
				}
			}
			
			if(this_max_float > max_float)
			{	max_float = this_max_float;
			}
			max_float_per_slice[j] = this_max_float;
			//and save the value off for calculating rescale slope later on
		}
	}
	return true;
}
//--------------------------------------------------------
double DRO_Phantom_PET::Calculate_Max_Float_For_Slice(int slice)
{
	return max_float_per_slice[slice];
}
//--------------------------------------------------------
bool DRO_Phantom_PET::CovertImageto_int16()
{	//1)image is already scaled 0-1.0, 2) scale each item -32768 to 32767 greyscale- short (2 byte); as per DICOM standard wih best resolution
	for(int i = 0; i < num_dro; i++)
	{
		//if(DRO_object_int16[i] >= 0)
		if(DRO_object[i] >= 0)
		{	DRO_object_int16[i] = (short)(DRO_object[i] * MAX_IMAGE_VALUE);
			
		}
		else	//< =0...allot for slight change in scale length
		{	DRO_object_int16[i] = (short)(- DRO_object[i] * MIN_IMAGE_VALUE);
		}
	}
	
	return true;
}
//--------------------------------------------------------
void DRO_Phantom_PET::AddCheckerBoardPatterns()
{
	AddCheckerBoard(true);		//add the volume
	AddCheckerBoard(false);		//add the area
}
//--------------------------------------------------------
void DRO_Phantom_PET::AddCheckerBoard(bool bvolume)
{	//works on an area in a single slice, or a volume over multiple slices

	//rather than scanning the entire object again, we calculate the extent and then scan over that area (ie, only do what is needed)
	
	//need to convert mm to pixel		
	int pixel_extent_x = (test_pattern_extentxy/2.0)/Get_dx();
	int pixel_extent_y = (test_pattern_extentxy/2.0)/Get_dy();
	int pixel_extent_z = (test_pattern_volume_extentz/2.0)/Get_dz();
	
	//x and y change if area rather than volume
	int startx, endx, starty, endy, startz, endz;
	
	if(bvolume)
	{	//convert x/y mm to pixel
		startx = 	(int)(test_pattern_volume_centerx / Get_dx()) - pixel_extent_x;
		endx = 		(int)(test_pattern_volume_centerx / Get_dx()) + pixel_extent_x + 1;
		starty = 	(int)(test_pattern_volume_centery / Get_dy()) - pixel_extent_y;
		endy = 		(int)(test_pattern_volume_centery / Get_dy()) + pixel_extent_y + 1;
		
		startz = 	test_pattern_slice - pixel_extent_z;	//convert pixels to mm
		endz = 		test_pattern_slice + pixel_extent_z + 1;
	}
	else	//single slice area, different centerpoint x/y
	{
		startx = 	(int)(test_pattern_area_centerx / Get_dx()) - pixel_extent_x;
		endx = 		(int)(test_pattern_area_centerx / Get_dx()) + pixel_extent_x + 1;
		starty = 	(int)(test_pattern_area_centery / Get_dy()) - pixel_extent_y;
		endy = 		(int)(test_pattern_area_centery / Get_dy()) + pixel_extent_y + 1;
		
		startz = test_pattern_slice;
		endz = startz + 1;	//1 slice
	}
	
	for(int inz = startz; inz < endz; inz++)
	{
	  for(int iny = starty; iny < endy; iny++)
	  {
		  for(int inx = startx; inx < endx; inx++)
		  {
			  int index = (inz * num_dy * num_dx) + (iny * num_dy) + inx;	//get the data item
			  bool bLow = bool ((inz + iny + inx) % 2); 
			  
			  DRO_object[index] = bLow ? CHECKER_BOARD_LOW : CHECKER_BOARD_HIGH;  
		  }
	  }
	}
}//--------------------------------------------------------
bool DRO_Phantom_PET::AddNoiseAndSmooth()
{	
	if(b_CalculateNoise)		//do noise and possibly smoothing
	{
	
		for(int noisy_slice = 0; noisy_slice < num_dz; 	noisy_slice++)		
		{	//output buffer for smoothing
			short * DRO_object_int16_out = NULL;	//final output format						
			
			DRO_object_int16_out = (short*) malloc(sizeof(short) * num_dro);	//9.9.11 - now just a slice and volume, dont need the whole thing - though simpler this way
			memset(DRO_object_int16_out, 0, (sizeof(short) * num_dro));	
			
			if(DRO_object_int16_out == NULL)
			{
				printf("Error in allocation DRO_object_int16_out! Aborting!\n");
				//this->~DRO_Phantom();	//dealloc through destructor
				exit(1);
			}
		
			//do the SINGLE SLICE OF NOISE
			if(noisy_slice >= 0)
			{
				//printf("\nApplying Gaussian Noise to slice %d.\n", noisy_slice+1);
				//bbfe 3.26.12 - no, limits noise to 0 - max
				//need to calc as an int, then rescale and recalc the max float per slice
				
				//AddNoise(AmountNoise, &DRO_object_int16[noisy_slice * num_dx * num_dy], 0, num_dx, 0, num_dy);	//amount of noise percentile as input -
			
				if(b_GaussianSmooth)
				{
					//printf("Applying Gaussian 2d circular convulation filter smoothing.\n");
					//gaussian smoothing - per slice
				
					GaussianSmooth2D(smooth_fwhm, &DRO_object_int16[noisy_slice * num_dx * num_dy], &DRO_object_int16_out[noisy_slice * num_dx * num_dy], noisy_slice, 0, num_dx, 0, num_dy);	//algorithm from LP
					memcpy(&DRO_object_int16[noisy_slice * num_dx * num_dy], &DRO_object_int16_out[noisy_slice * num_dx * num_dy], sizeof(short) * num_dx * num_dy);	//copy back the result
				}
			}
			free(DRO_object_int16_out);
		}
	}
	
	return true;
}//--------------------------------------------------------
void DRO_Phantom_PET::AddNoise_double(float calibbration_in, double* in)
{	//operates on scaled short data representation
	
	//lets use calibration as a float, 0..1, as a percentile of noise to apply
	if(calibbration_in <= 0 || calibbration_in > 1.0)
	{
		printf("DRO_phantom::AddNoise: Error with calibration: %f! Noise calibration should be a percentage to apply between 0 and 1.\n", calibbration_in);
		//return;
	}
	
	double amount;
	double calibration = calibbration_in * MAX_IMAGE_VALUE;			//calculate in pixel space, not object

	double	  g_sd_calib = calibration * (2.0*sqrt(2.0*log(2)));	//FWHM
	double	  g_sd_2x_calib = g_sd_calib * 2.0;						//get plus or minus
	g_sd_2x_calib = pow(g_sd_2x_calib, 2);							//and square to get exponetial
	int total_negative = 0;
	int xtra_iter_count = 0;
	int xtra_elements_count = 0;
	int xtra_elements_atmax_count = 0;
		
	for(int noisy_slice = 0; noisy_slice < num_dz; 	noisy_slice++)		
	{
		for(int iny = 0; iny < num_dy; iny++)
		{
			for(int inx = 0; inx < num_dx; inx++)
			{
				int index = (noisy_slice * num_dx * num_dy) + (iny * num_dy) + inx;
				//int index = (iny * num_dy) + inx;
				
				//dont apply to background
				//this would conflict wil the cold voxel, if we allowed them to co-exist - which we dont
				if(in[index] > 0)	
				{	//we need to apply an amount of noise independent from the pixel value
					
					//applies pixel variation based on spec’d standard deviation for max pixel value of image
					//take poisson of a larger number to allot for float scaling - (this multiplier/divisor could be a constant like 100)
					//take poisson of a multipe of calibration, dividing by that value to gain accuracy. Subract off to center around value, then multiply up to scale
					
					//amount = (((Distribution_obj.poisson(calibration* calibration)/ calibration) - calibration)  * calibration);	//.0613098
					
					//THIS WORKS 3.27 -  AS A DOUBLE	//.0613098
//					amount = (Distribution_obj.poisson(calibration*10000));
//					amount /= 10000.0;
//					amount -= calibration;
//					amount *= max_imageactivity;

					//tends to scale down a bit, likey due to negative values
					// need to compensate for the min in he rescaling - and thus store min value per slice out
					int it = 0;
					do
					{
					  amount = (Distribution_obj.poisson(g_sd_2x_calib));//need to multiple in FWHM	//.05
					  amount -= g_sd_2x_calib;
					  amount /= (double)MAX_IMAGE_VALUE;

					  amount /= (double)(it+1);	//force it to center if its outside on subsequent iterations
				  
					  amount = in[index] + amount;
					  
					  if(it >= 1)
						  xtra_iter_count++;	//track how many xtra times through
						  
					  if(it == 1)				//track how many pixel ements have to be retried
						  xtra_elements_count++;
						  
					  if(it == 4)				//track how many pixel ements tried max times
						  xtra_elements_atmax_count++;
					  
					 } while (amount < 0 && it++ < 4);	//2//3//4		//~68 lost
					 
					 //creates a little bit of rescale bias, but is minimal
					 //needed to assure non-negativity - though 10-50 per image is ideal
					 //NOTE: changes the distribution when you resample outlier tails - or reset them to 0 if negative! - though seems minor if iterations kept very low N2/it3 or N10/it4  :best when negs == 10-400
					
					if(amount < 0)	
					{	amount = 0;		
						total_negative++;	//track how many reset
					}
					
					/*DISCLAIMER: The noise generated in this image is not representative of noise that would be encountered in a clinal setting. 
					Instead poisson noise has been applied to the image pixel values in order to obtain a given mean standard deviation in pixel value 
					from the true value based on a randomized poisson & gaussian distribution generator. Pixel values are assured non-negative and allowed 
					to scale up the maximum image value. Noise for a given pixel is not relative to that pixels intensity, rather being appleid uniformly across the image intensity.
					As a given realization's noise is dependent on a specific instance of the psuedo-random number genrator,
					each and every run (or realization) will have a different noise distribution (for a given random seed).					
					*/

					in[index] = (amount);	
				}
			 }
		}
	}
	printf("DRO_phantom: STATS: total elements required extra tries: %d.\n", xtra_elements_count);
	printf("DRO_phantom: STATS: total elements required extra tries till max allowed: %d.\n", xtra_elements_atmax_count);
	printf("DRO_phantom: STATS: total xtra iterations required: %d.\n", xtra_iter_count);
	printf("DRO_phantom: STATS:total negative reset to 0: %d.\n", total_negative);
		
	//and will need to recalculate maxfoalt_per_slice!!! in order to get rescale per slice correct
}
//--------------------------------------------------------
void DRO_Phantom_PET::AddNoise(float calibration, short* in, int startx, int endx, int starty, int endy)
{	//operates on scaled short data representation
	
	//lets use calibration as a float, 0..1, as a percentile of noise to apply
	if(calibration <= 0 || calibration > 1.0)
	{
		printf("DRO_phantom::AddNoise: Error with calibration: %f! Noise calibration should be a percentage to apply between 0 and 1. ABORTING Noise calculation.\n", calibration);
		return;
	}
	
	//set the value in pixel variation mean
	calibration = calibration * MAX_IMAGE_VALUE;
	
	double amount;
	const double max_value = MAX_IMAGE_VALUE;						//do we want 4.11 to be the max for other parts of the image?
	
	for(int iny = starty; iny < endy; iny++)
	{
		for(int inx = startx; inx < endx; inx++)
		{
			//int index = (slice * num_dx * num_dy) + (iny * num_dy) + inx;
			int index = (iny * num_dy) + inx;
			
			//dont apply to background
			//this would conflict wil the cold voxel, if we allowed them to co-exist - which we dont
			if(in[index] > 0)	
			{	//we need to apply an amount of noise independent from the pixel value
				
				//applies pixel variation based on spec’d standard deviation for max pixel value of image
				//take poisson of a larger number to allot for float scaling - (this multiplier/divisor could be a constant like 100)
				//take poisson of a multipe of calibration, dividing by that value to gain accuracy. Subract off to center around value, then multiply up to scale
				amount = (((Distribution_obj.poisson(calibration* calibration)/ calibration) - calibration)  * calibration);
				amount = in[index] + amount;	
				amount = amount + .5;								//allot for correct trucation/round off
				
				if(amount < 0)	
					amount = 0;
				else if (amount > max_value)						//if we want to truncate values at 4.0 rather than 4.11 - this is the place to do it -currently 4.11
					amount = max_value;
					
				in[index] = (short) (amount);	//cast into a short
			}
		 }
	}
}
//--------------------------------------------------------
void DRO_Phantom_PET::GaussianSmooth2D(double g_fwhm, short* in, short* out, int slice, int startx, int endx, int starty, int endy)
{	//algorithm from LP implementation - uses a circular filter for smoothing
	//slice implied by in parameter

	double kernel_sum = 0.0;
	double sum_dro = 0.0;
			
	if(bGaussian_kernel_inited == false)
	{	//allocate and initialize one time use variables for the smoothing algorithm - re-uses these ariables each iteration once set
		bGaussian_kernel_inited = true;
		GaussianSmooth2D_Allocate(g_fwhm);
	}
	
	//ideally the sum needs to be calculated for each convolution iteration (ie pixel), as it changes at the edges!
	//now brute force apply the kernel over the image
	//loop the image
	for(int y_id = starty; y_id < endy; y_id++)
	{	for(int x_id = startx; x_id < endx; x_id++)
		{
			kernel_sum = 0.0;
			
			//loop the kernel
			//first get the sum of this value of the kernel, 
			//next make a new normalized kernel
			//then we apply it
			for(int k_y = 0; k_y < num_y_kernel; k_y++)
			{	for(int k_x = 0; k_x < num_x_kernel; k_x++)
				{
					//iterate the kernel over the image, but only if its a valid part of the image
					if((k_x - num_x_kernel/2 + x_id >= startx) && (k_x - (num_x_kernel/2) + x_id < endx))
					{
						if((k_y - num_y_kernel/2 + y_id >= starty) && (k_y - (num_y_kernel/2) + y_id < endy))
						{
							//we need to make a kernel sum and normalize the kernel first
							kernel_sum += g_kernel[k_y * num_y_kernel + k_x];
						}
					}
				}
			}
			
			//reset the normalized kernel
			memset(g_kernel_new, 0, sizeof(double) * num_y_kernel * num_x_kernel);
			//normalize the new kernel
			for(int i = 0; i < num_y_kernel * num_x_kernel; i++)
			{
				if(g_kernel[i] != 0.0)
				{	g_kernel_new[i] = g_kernel[i]/kernel_sum;
				}
			}
			
			sum_dro = 0.0;
			//and then apply the normalized kernel to the pixel values- get a new 'smoothed' value for the pixel
			for(int k_y = 0; k_y < num_y_kernel; k_y++)
			{	for(int k_x = 0; k_x < num_x_kernel; k_x++)
				{
					//iterate the kernel over the image, but only if its a valid part of the image
					if((k_x - num_x_kernel/2 + x_id >= startx) && (k_x - (num_x_kernel/2) + x_id < endx))
					{
						if((k_y - num_y_kernel/2 + y_id >= starty) && (k_y - (num_y_kernel/2) + y_id < endy))	//would be slightly faster with this test otside of loop....
						{							
							//needs additional bounds checking??
							//sum_dro += in[(y_id * num_dy) + ((k_y - num_y_kernel/2) * num_dy) + x_id + (k_x - num_x_kernel/2)] * g_kernel_new[k_y * num_y_kernel + k_x];
							sum_dro += in[((k_y - num_y_kernel/2 + y_id) * num_dy) + x_id + (k_x - num_x_kernel/2)] * g_kernel_new[k_y * num_y_kernel + k_x];
						}
					}
				}
			}
			
			//do we need limiting? no- smoothing will only average, thus the new value is always within min & max
			//and assign it to the output buf
			out[y_id * num_dy + x_id] = sum_dro;			
		}	
	}
}
//--------------------------------------------------------
void DRO_Phantom_PET::GaussianSmooth2D_Allocate(double g_fwhm)
{		
	//Convert FWHM to SD for the Gaussian:
	  g_sd = g_fwhm/(2.0*sqrt(2.0*log(2)));
	  g_sd_2x = g_sd * 2.0;
	  
	  // The Gaussian will go out to 2 standard deviations, so we make vectors
	  // containing the x, y, and z values for the Gaussian input:
	  //x_kernel_vals = 0:dx:2*g_sd;
	  num_x_kernel = (int)((((2.0 * g_sd)/ Get_dx()) + 1) * 2) + 1;	
	  num_y_kernel = (int)((((2.0 * g_sd)/ Get_dy()) + 1) * 2) + 1;
	  //the normalized kernel, which changes at the edges, as the excluded values are not included in the normalization calculation
	  
	  x_kernel_vals = (double*) malloc(sizeof(double) * num_x_kernel);
	  y_kernel_vals = (double*) malloc(sizeof(double) * num_y_kernel);
	  g_kernel = (double*) malloc(sizeof(double) * num_y_kernel * num_x_kernel);		//kernel
	  g_kernel_new = (double*) malloc(sizeof(double) * num_y_kernel * num_x_kernel);	//normalized kernel
	  
	  if(x_kernel_vals == NULL || y_kernel_vals == NULL || g_kernel == NULL || g_kernel_new == NULL)
	  {
		  printf("Error in allocation (-GaussianSmooth2D - new-)! Aborting!\n");
		  //this->~DRO_Phantom();	//dealloc through destructor
		  exit(1);
	  }
	  
	  printf("GaussianSmooth2D: The kernel size is x: %d and y: %d\n", num_x_kernel, num_y_kernel);
	  
	  //-------the X separable values---------
	  //these should be stored out...
	  //preload the endpoints
	  x_kernel_vals[0] = -g_sd_2x;
	  x_kernel_vals[num_x_kernel-1] = g_sd_2x;
	  for(int i = 1; i < num_x_kernel-1 ; i++)
	  {
		  if(i < num_x_kernel/2)
		  {	x_kernel_vals[i] = -((num_x_kernel/2) - i) * Get_dx();
		  }
		  else
		  {	x_kernel_vals[i] = (i - (num_x_kernel/2)) * Get_dx();
		  }
	  }
	  
	  //------------do the Y
	  //preload the endpoints
	  y_kernel_vals[0] = -g_sd_2x;
	  y_kernel_vals[num_y_kernel-1] = g_sd_2x;
	  for(int i = 1; i < num_y_kernel-1 ; i++)
	  {
		  if(i < num_y_kernel/2)
		  {	y_kernel_vals[i] = -((num_y_kernel/2) - i) * Get_dy();
		  }
		  else
		  {	y_kernel_vals[i] = (i - (num_y_kernel/2)) * Get_dy();
		  }
	  }
	  
	  // Now, create the convolution kernel. Start with a storage element:
	  memset(g_kernel, 0, sizeof(double) * num_y_kernel * num_x_kernel);
	  // Go over every entry in the g_kernel and compute the Gaussian weight:

	  for(int y_kernel_index = 0; y_kernel_index < num_y_kernel; y_kernel_index++)
	  {
		  for(int x_kernel_index = 0; x_kernel_index < num_x_kernel; x_kernel_index++)
		  {
			  double d = sqrt(pow(x_kernel_vals[x_kernel_index], 2) + pow(y_kernel_vals[y_kernel_index], 2) );
			  
			  if( d <= g_sd_2x)
			  {
				  //g_kernel(y_kernel_index,x_kernel_index) = n_dist(d,0,g_sd);	//bfe- normal distribution....
				  //function output_val = n_dist(x , mu , sigma )
				  //output_val = exp(-(x-mu)^2/(2*sigma^2)) / (sigma*sqrt(2*pi));

				  double k_value = exp( - pow((d-0.0), 2) / (2.0 * pow(g_sd, 2))) / (g_sd * sqrt(2.0 * M_PI));
				  
				  g_kernel[y_kernel_index * num_y_kernel + x_kernel_index] = k_value;	
			  }
		  }
	  }
}
//--------------------------------------------------------
bool DRO_Phantom_PET::WriteDROfile()
{
	//testing only - write out raw data
	FILE* fdes = NULL;
	if ((fdes = fopen(OutputFileName, "w")) == NULL)
	{
		printf("DRO_phantom::WriteDROfile: Error opening %s for writing!\n", OutputFileName);
		//this->~DRO_Phantom();	//dealloc through destructor
		exit(1);
	}
	
	//fwrite(DRO_object_int16, sizeof(short), num_dro, fdes);		// short data
	fwrite(DRO_object, sizeof(double), num_dro, fdes);			//double data
	//or write a single slice
	//fwrite(&DRO_object[39 * num_dx * num_dy], sizeof(double), num_dx * num_dy, fdes);			//double data
	printf("DRO_phantom::Wrote to file %s\n", OutputFileName);
	fclose(fdes);	

	return true;
}
//--------------------------------------------------------
bool DRO_Phantom_PET::AreInputValuesValid()
{	//check the input values for allowable values

	const double min_emission_activity = 0.0;
	const double max_emission_activity = MAX_IMAGE_VALUE;

	if(	body_activity < min_emission_activity || body_activity > max_emission_activity ||
		center_rod_activity < min_emission_activity || center_rod_activity > max_emission_activity ||
		sphere_activity < min_emission_activity || sphere_activity > max_emission_activity ||
		hot_voxel_activity < min_emission_activity || hot_voxel_activity > max_emission_activity ||
		background_activity < min_emission_activity || background_activity > max_emission_activity)
	{
	
		printf(" AreInputValuesValid> !!! Error: Emission activity values must be between %lf and %lf\n", min_emission_activity, max_emission_activity);
		return false;
	}

	return DRO_Phantom::AreInputValuesValid();
}
//--------------------------------------------------------
bool DRO_Phantom_PET::AreParametersWithinLimits()
{
	
	if(b_CalculateNoise && b_show_test_pattern)
	{	//not allowed, as typically results in noisy/smoothed test pattern
		printf(" AreInputValuesValid> !!! Error: Noise and the test pattern are exclusive, and must not be run together.\n");
		return false;
	}
//	if(b_CalculateNoise && b_ShowHotVoxel)
//	{	//and noisy_slice == hotvoxel slice location
//		int slicez = HOTVOXEL_SLICE;	//this needs to be adjusted based on the number of slices if we want it to appear in the same position relative (for runs with various # slices)
//		slicez = (int) (num_dz * (slicez/109.0));	//might want to set directly to slice closest to -30mm
//
//		if(noisy_slice == slicez)	//you cannot blur the hot/cold voxel- either turn off hot voxel or choose a different noisy slice
//		{
//			printf(" AreInputValuesValid> !!! Error: The noisy slice (%d) conflicts with the hot/cold voxel slice, and they must be exclusive.\n", noisy_slice+1);
//			return false;
//		}
//	}

	if(b_show_test_pattern)
	{	//check test pattern
	  
		int pixel_extent_x = (test_pattern_extentxy/2.0)/Get_dx();
		int pixel_extent_y = (test_pattern_extentxy/2.0)/Get_dy();
		int pixel_extent_z = (test_pattern_volume_extentz/2.0)/Get_dz();
		int startx, endx, starty, endy, startz, endz, startx_area, endx_area, starty_area, endy_area;
	  
		//convert x/y mm to pixel
		startx = 	(int)(test_pattern_volume_centerx / Get_dx()) - pixel_extent_x;
		endx = 		(int)(test_pattern_volume_centerx / Get_dx()) + pixel_extent_x + 1;
		starty = 	(int)(test_pattern_volume_centery / Get_dy()) - pixel_extent_y;
		endy = 		(int)(test_pattern_volume_centery / Get_dy()) + pixel_extent_y + 1;
		  
		startz = 	test_pattern_slice - pixel_extent_z;	//convert pixels to mm
		endz = 		test_pattern_slice + pixel_extent_z + 1;

		startx_area = 	(int)(test_pattern_area_centerx / Get_dx()) - pixel_extent_x;
		endx_area = 	(int)(test_pattern_area_centerx / Get_dx()) + pixel_extent_x + 1;
		starty_area = 	(int)(test_pattern_area_centery / Get_dy()) - pixel_extent_y;
		endy_area = 	(int)(test_pattern_area_centery / Get_dy()) + pixel_extent_y + 1;
		
		//check z slice extents
		if(test_pattern_slice < 0 || test_pattern_slice > num_dz)	//this should be tested after we calculate the number of slices here
		{
			printf(" AreInputValuesValid> !!! Error: The slice to add test pattern to (%d) needs to be a value between 1 and maximum slice %d.\n", test_pattern_slice, num_dz);
			return false;
		}
		if(startz < 0 || endz > num_dz)
		{
			printf(" AreInputValuesValid> !!! Error: The test pattern volume slices (min: %d, max: %d) need to be a value between 0 and maximum slice %d.\n", startz, endz, num_dz);
			return false;
		}	  
		if(startz > endz)
		{
			printf(" AreInputValuesValid> !!! Error: The test pattern volume slices needs min: %d to be less than max: %d.\n", startz, endz);
			return false;
		}
		
		//check volume x,y
		if(startx < 0 || endx > num_dx)
		{
			printf(" AreInputValuesValid> !!! Error: The test pattern volume x values (%d, %d) need to be between 0 and maximum x %d.\n", startx, endx, num_dx);
			return false;
		}	  
		if(startx > endx)
		{
			printf(" AreInputValuesValid> !!! Error: The test pattern volume x needs min x: %d to be less than max x: %d.\n", startx, endx);
			return false;
		}
		
		if(starty < 0 || endy > num_dy)
		{
			printf(" AreInputValuesValid> !!! Error: The test pattern volume y values (%d, %d) need to be between 0 and maximum x %d.\n", starty, endy, num_dy);
			return false;
		}	  
		if(starty > endy)
		{
			printf(" AreInputValuesValid> !!! Error: The test pattern volume y needs min x: %d to be less than max x: %d.\n", starty, endy);
			return false;
		}
		
			
		//check area x,y
		if(startx_area < 0 || endx_area > num_dx)
		{
			printf(" AreInputValuesValid> !!! Error: The test pattern area x values (%d, %d) need to be between 0 and maximum x %d.\n", startx_area, endx_area, num_dx);
			return false;
		}	  
		if(startx_area > endx_area)
		{
			printf(" AreInputValuesValid> !!! Error: The test pattern area x needs min x: %d to be less than max x: %d.\n", startx_area, endx_area);
			return false;
		}
		
		if(starty_area < 0 || endy_area > num_dy)
		{
			printf(" AreInputValuesValid> !!! Error: The test pattern area y values (%d, %d) need to be between 0 and maximum x %d.\n", starty_area, endy_area, num_dy);
			return false;
		}	  
		if(starty_area > endy_area)
		{
			printf(" AreInputValuesValid> !!! Error: The test pattern area y needs min x: %d to be less than max x: %d.\n", starty_area, endy_area);
			return false;
		}
	}
	//check noisy slice
	//also make sure noisy slice is not slice 40 if showing hot/cold voxel
	if(b_CalculateNoise)
	{
		//slice internally store 0..n-1
	/*	if(noisy_slice < 0 || noisy_slice > num_dz)	//this should be tested after we calculate the number of slices here
		{
			printf(" AreInputValuesValid> !!! Error: The slice to add noise to (%d) needs to be a value between 1 and maximum slice %d.\n", noisy_slice+1, num_dz);
			return false;
		}*/
		if(AmountNoise <= 0 || AmountNoise > 1.0)
		{
			printf(" AreInputValuesValid> !!! Error: The amount of noise (%f) needs to be a value between 0 and 1.\n", AmountNoise);
			//return false;
		}
		
		if(b_GaussianSmooth)
		{		
		  if(smooth_fwhm <= 0 || smooth_fwhm > 100.0)
		  {
			  printf(" AreInputValuesValid> !!! Error: The FWHM for smoothing (%f) needs to be a value larger than 0 and less than 100.\n", smooth_fwhm);
			  return false;
		  }
		}
	
	}
	return true;
}

























