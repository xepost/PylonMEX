#include <mex.h>
#include <PylonMEX\PylonMEX.h>
using namespace std;
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	try {
		if (!gPylonMEXInstance) {
			throw "PylonMEX is not initialized.";
		}
		vector<bool> ro = gPylonMEXInstance->PropertyROStates();
		plhs[0] = mxCreateLogicalMatrix(ro.size(), 1);
		mxLogical* out = (mxLogical*)mxGetData(plhs[0]);
		for (int n = 0; n < ro.size(); n++) {
			out[n] = ro[n];
		}
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