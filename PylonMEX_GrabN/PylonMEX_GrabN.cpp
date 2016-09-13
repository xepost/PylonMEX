#include <mex.h>
#include <PylonMEX\PylonMEX.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	double timeoutMS = 5000;
	int N = 1;
	int Ngrabbed;
	if (nrhs > 0) {
		N = mxGetScalar(prhs[0]);
	}
	if (nrhs > 1) {
		timeoutMS = mxGetScalar(prhs[1]);
	}
	try {
		if (!gPylonMEXInstance) {
			throw "PylonMEX is not initialized.";
		}
		bool grabbedOK;
		if(nlhs>1)
			grabbedOK = gPylonMEXInstance->GrabN(N, &plhs[0], &plhs[1], &Ngrabbed, timeoutMS);
		else
			grabbedOK = gPylonMEXInstance->GrabN(N, &plhs[0], nullptr, &Ngrabbed, timeoutMS);
		if (!grabbedOK) {
			mexPrintf("PylonMEX_GrabN(): Timeout during grab\n");
		}
		if(nlhs>2)
			plhs[2] = mxCreateLogicalScalar(grabbedOK);
		if(nlhs>3)
			plhs[3] = mxCreateDoubleScalar(Ngrabbed);
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