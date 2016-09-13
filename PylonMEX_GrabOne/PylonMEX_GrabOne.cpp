#include <mex.h>
#include <PylonMEX\PylonMEX.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	double timeoutMS = 5000;
	if (nrhs > 0) {
		timeoutMS = mxGetScalar(prhs[0]);
	}
	try {
		if (!gPylonMEXInstance) {
			throw "PylonMEX is not initialized.";
		}
		double now;
		plhs[0] = gPylonMEXInstance->Grab(timeoutMS,&now);
		if(nlhs>1)
			plhs[1] = mxCreateDoubleScalar(now);

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