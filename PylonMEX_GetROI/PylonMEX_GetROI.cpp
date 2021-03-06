#include <mex.h>
#include <PylonMEX\PylonMEX.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	try {
		if (!gPylonMEXInstance) {
			throw "PylonMEX is not initialized.";
		}
		plhs[0] = mxCreateDoubleMatrix(1, 4, mxREAL);
		size_t x, y, w, h;
		gPylonMEXInstance->GetROI(&x,&y,&w,&h);

		mxGetPr(plhs[0])[0] = x;
		mxGetPr(plhs[0])[1] = y;
		mxGetPr(plhs[0])[2] = w;
		mxGetPr(plhs[0])[3] = h;
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