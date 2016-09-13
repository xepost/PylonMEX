/****************************************************************************
PylonMEX_SetProperty(name, value)
Set property to specified value
*****************************************************************************/

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
		if (nrhs < 2) {
			throw RUNTIME_EXCEPTION("SetProperty(name,val) must have two inputs");
		}
		if (!mxIsChar(prhs[0])) {
			throw RUNTIME_EXCEPTION("SetProperty(name,val) name must be char array");
		}
		mxGetString(prhs[0], name, 255);
		switch (gPylonMEXInstance->PropertyType(name)) {
		case intfIInteger:
			if (!mxIsScalar(prhs[1])) {
				throw RUNTIME_EXCEPTION("value must be scalar integer");
			}
			gPylonMEXInstance->SetProperty(name, (int64_t)mxGetScalar(prhs[1]));
			break;
		case intfIBoolean:
			if (!mxIsScalar(prhs[1])) {
				throw RUNTIME_EXCEPTION("value must be scalar bool");
			}
			gPylonMEXInstance->SetProperty(name, (bool)mxGetScalar(prhs[1]));
			break;
		case intfIFloat:
			if (!mxIsScalar(prhs[1])) {
				throw RUNTIME_EXCEPTION("value must be scalar double");
			}
			gPylonMEXInstance->SetProperty(name, mxGetScalar(prhs[1]));
			break;
		case intfIString:
			if (!mxIsChar(prhs[1])) {
				throw RUNTIME_EXCEPTION ("value must be char");
			}
			mxGetString(prhs[1], val, 255);
			gPylonMEXInstance->SetProperty(name, string(val));
			break;
			
		case intfIEnumeration:
			if (!mxIsChar(prhs[1])) {
				throw RUNTIME_EXCEPTION ("value must be char");
			}
			mxGetString(prhs[1], val, 255);
			gPylonMEXInstance->SetProperty(name, string(val));
			break;
		default:
			throw "type error";
		}
	}
	catch (const GenericException &e)
	{
		std::ostringstream Err;
		// Error handling.
		Err << "An exception occurred while Calling:" << std::endl
			<< "PylonMEX_SetProperty('" << name << "',value)" << endl
			<< e.GetDescription() << std::endl;
		mexErrMsgTxt(Err.str().c_str());
	}

}