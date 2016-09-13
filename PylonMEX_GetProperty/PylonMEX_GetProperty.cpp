/*
value = PylonMEX_GetProperty(name)
get property value by name
*/

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

		if (typ == intfIBoolean) {
			//mexPrintf("in bool\n");
			bool val = false;
			//mexPrintf("before val:%d\n", (int)val);
			gPylonMEXInstance->GetProperty(name, &val);
			//mexPrintf("after val:%d\n", (int)val);
			plhs[0] = mxCreateLogicalScalar(val);
		}
		else if (typ == intfIInteger) {
			int64_t val;
			gPylonMEXInstance->GetProperty(name, &val);
			plhs[0] = mxCreateDoubleScalar(val);
		}
		else if (typ == intfIFloat) {
			double val;
			gPylonMEXInstance->GetProperty(name, &val);
			plhs[0] = mxCreateDoubleScalar(val);
		}
		else if (typ == intfIString) {
			string val;
			gPylonMEXInstance->GetProperty(name, &val);
			plhs[0] = mxCreateString(val.c_str());
		}
		else if (typ == intfIEnumeration) {
			string val;
			gPylonMEXInstance->GetProperty(name, &val);
			plhs[0] = mxCreateString(val.c_str());
		}
		else {
			plhs[0] = mxCreateDoubleScalar(mxGetNaN());
		}

	}
	catch (const GenericException &e)
	{
		std::ostringstream Err;
		// Error handling.
		Err << "An exception occurred while Calling:" << std::endl
			<< "PylonMEX_GetProperty('" << name << "')" << endl
			<< e.GetDescription() << std::endl;
		mexErrMsgTxt(Err.str().c_str());
	}

}