#include "MatlabTime.h"



//todo unix code, cant use chrono because matlab now() returns datenum relative to local time not UTC
static const WORD cumdays[] = { 0, 0,31,59,90,120,151,181,212,243,273,304,334 };

double Matlab_now() {
	double mNow;
#ifdef _WIN32
	SYSTEMTIME lt;
	GetLocalTime(&lt);

	mNow = (double)(365 * lt.wYear + cumdays[lt.wMonth] + lt.wDay
		+ lt.wYear / 4 - lt.wYear / 100 + lt.wYear / 400
		+ (lt.wYear % 4 != 0) - (lt.wYear % 100 != 0) + (lt.wYear % 400 != 0))
		+ (double)lt.wHour / 24.0
		+ (double)lt.wMinute / (24.0*60.0)
		+ (double)lt.wSecond / 86400.0
		+ (double)lt.wMilliseconds / 1000.0 / 86400.0;

	if (lt.wMonth > 2 && lt.wYear % 4 == 0 && (lt.wYear % 100 != 0 || lt.wYear % 400 == 0)) {
		++mNow;
	}
#endif
	return mNow;

}

double SYSTEMTIME_to_datenum(SYSTEMTIME lt) {
	double mNow;
	mNow = (double)(365 * lt.wYear + cumdays[lt.wMonth] + lt.wDay
		+ lt.wYear / 4 - lt.wYear / 100 + lt.wYear / 400
		+ (lt.wYear % 4 != 0) - (lt.wYear % 100 != 0) + (lt.wYear % 400 != 0))
		+ (double)lt.wHour / 24.0
		+ (double)lt.wMinute / (24.0*60.0)
		+ (double)lt.wSecond / 86400.0
		+ (double)lt.wMilliseconds / 1000.0 / 86400.0;

	if (lt.wMonth > 2 && lt.wYear % 4 == 0 && (lt.wYear % 100 != 0 || lt.wYear % 400 == 0)) {
		++mNow;
	}
	return mNow;
}