/******************************************************************************
[PropType, Opt, increment] = PylonMEX_PropertyAllowedValues(name)
Get allowed values for property specified by name

Input:
	name: string specifying property name
Output:
	PropType: string listing property type ('intfIInteger, intfIFloat,...)
	Opt: if property is numeric then opt is the value limits [MinValue, MaxValue]
		 if property is intfIEnumeration then opt = {'FirstValue','SecondValue',...}
		 if bool then [0,1]
	increment: if property has increment it is specified here

*******************************************************************************/

#include <mex.h>
#include <PylonMEX\PylonMEX.h>

using namespace std;
using namespace GenApi;
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	char name[255];
	char val[255];
	try {
		if (!gPylonMEXInstance) {
			throw RUNTIME_EXCEPTION("PylonMEX is not initialized.");
		}
		if (nrhs < 1) {
			throw RUNTIME_EXCEPTION("GetProperty(name) must have one input");
		}
		if (!mxIsChar(prhs[0])) {
			throw RUNTIME_EXCEPTION("GetProperty(name) name must be char array");
		}
		mxGetString(prhs[0], name, 255);
		EInterfaceType typ = gPylonMEXInstance->PropertyType(name);
		double low, high, increment;
		GenApi::StringList_t opts;
		string typ_str = TypeToString(typ);
		plhs[0] = mxCreateString(typ_str.c_str());

		if (typ == intfIBoolean) {
			low = 0;
			high = 1;
			increment = 1;
			plhs[1] = mxCreateDoubleMatrix(1, 2, mxREAL);
			mxGetPr(plhs[1])[0] = low;
			mxGetPr(plhs[1])[1] = high;
			plhs[2] = mxCreateDoubleScalar(increment);
		}
		else if (typ == intfIInteger || typ == intfIFloat) {
			gPylonMEXInstance->PropertyLimits(name, &low, &high, &increment);
			plhs[1] = mxCreateDoubleMatrix(1, 2, mxREAL);
			mxGetPr(plhs[1])[0] = low;
			mxGetPr(plhs[1])[1] = high;
			plhs[2] = mxCreateDoubleScalar(increment);
		}
		else if (typ == intfIString) {
			plhs[1] = mxCreateCellMatrix(0, 0);
			plhs[2] = mxCreateDoubleMatrix(0, 0, mxREAL);
		}
		else if (typ == intfIEnumeration) {
			gPylonMEXInstance->EnumeratePropertyValues(name, opts);
			plhs[1] = mxCreateCellMatrix(opts.size(), 1);
			for (int n = 0; n < opts.size(); ++n) {
				mxSetCell(plhs[1], n, mxCreateString(opts[n].c_str()));
			}
			plhs[2] = mxCreateDoubleMatrix(0, 0, mxREAL);
		}
		else {
			plhs[1] = mxCreateDoubleMatrix(0, 0, mxREAL);
			plhs[2] = mxCreateDoubleMatrix(0, 0, mxREAL);
		}
	}
	catch (const GenericException &e)
	{
		std::ostringstream Err;
		// Error handling.
		Err << "An exception occurred while Calling:" << std::endl
			<< "PylonMEX_PropertyAllowedValues('" << name << "')" << endl
			<< e.GetDescription() << std::endl;
		mexErrMsgTxt(Err.str().c_str());
	}

}