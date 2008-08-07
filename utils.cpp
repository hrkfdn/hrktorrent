#include "hrktorrent.h"

float 
Round(float Value, int NumPlaces)
{
	float Factor;
	int Temp;
   
	Factor = pow(10.0, NumPlaces);
	Temp = (int)(Value * Factor + 0.5f);
	return Temp / Factor;
}
