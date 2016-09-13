#include <mex.h>
#include <PylonMEX\PylonMEX.h>
using namespace std;
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	try {
		if (!gPylonMEXInstance) {
			throw "PylonMEX is not initialized.";
		}
		vector<string> names = gPylonMEXInstance->PropertyNames();
		plhs[0] = mxCreateCellMatrix(names.size(), 1);
		for (int n = 0; n < names.size(); n++) {
			mxSetCell(plhs[0], n, mxCreateString(names[n].c_str()));
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