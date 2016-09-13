#include <mex.h>
#include <PylonMEX\PylonMEX.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	try {
		if (!gPylonMEXInstance) {
			throw "PylonMEX is not initialized.";
		}
		if (nrhs != 1 && nrhs != 4) {
			throw "input must be either ...SetROI([x,y,w,h]) of ...SetROI(x,y,w,h) ";
		}
		double x, y, w, h;
		if (nrhs == 1) {
			int m = mxGetM(prhs[0]);
			int n = mxGetN(prhs[0]);
			if (m*n != 4) {
				throw "input must be ...SetROI([x,y,w,h])";
			}
			x = mxGetPr(prhs[0])[0];
			y = mxGetPr(prhs[0])[1];
			w = mxGetPr(prhs[0])[2];
			h = mxGetPr(prhs[0])[3];
		}
		else {
			if (!mxIsScalar(prhs[0]) || !mxIsScalar(prhs[1]) || !mxIsScalar(prhs[2]) || !mxIsScalar(prhs[3])) {
				throw "inputs ...SetROI(x,y,w,h) must all be scalar";
			}
			x = mxGetScalar(prhs[0]);
			y = mxGetScalar(prhs[1]);
			w = mxGetScalar(prhs[2]);
			h = mxGetScalar(prhs[3]);
		}
		gPylonMEXInstance->SetROI(round(x), round(y), round(w), round(h));
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