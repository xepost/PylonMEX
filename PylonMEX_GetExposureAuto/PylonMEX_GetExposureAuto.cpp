#include <mex.h>
#include <PylonMEX\PylonMEX.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	try {
		if (!gPylonMEXInstance) {
			throw "PylonMEX is not initialized.";
		}
		plhs[0] = mxCreateLogicalScalar(gPylonMEXInstance->GetExposureAuto());
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