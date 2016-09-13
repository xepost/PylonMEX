#include <mex.h>
#include <PylonMEX\PylonMEX.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	try {
		if (!gPylonMEXInstance) {
			throw "PylonMEX is not initialized.";
		}
		if (nrhs < 1) {
			throw "Must had one input";
		}
		if (!mxIsScalar(prhs[0]) || !mxIsDouble(prhs[0])) {
			throw "input must be scalar double";
		}
		gPylonMEXInstance->SetExposure(mxGetScalar(prhs[0]));
	}
	catch (const GenericException &e)
	{
		std::ostringstream Err;
		// Error handling.
		Err << "An exception occurred." << std::endl
			<< e.GetDescription() << std::endl;
		mexErrMsgTxt(Err.str().c_str());
	}

}