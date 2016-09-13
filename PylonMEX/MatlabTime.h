#ifndef _MATLABTIME_H_
#define _MATLABTIME_H_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

//Return current time in the MATLAB datenum format
//(uses local time because that's what MATLAB uses)
double Matlab_now();
double SYSTEMTIME_to_datenum(SYSTEMTIME lt);

#endif //end _MATLABTIME_H_