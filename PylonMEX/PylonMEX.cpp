#include "PylonMEX.h"
#include <ctime>

API PylonMEX* gPylonMEXInstance = nullptr;


const char* SpecProp[] = { "ExposureTime",
							"ExposureAuto",
							"PixelFormat",
							"Width",
							"Height",
							"OffsetX",
							"OffsetY",
							"CenterX",
							"CenterY",
							"BinningHorizontal",
							"BinningVertical",
							"DecimationHorizontal",
							"DecimationVertical" };

std::vector<std::string> SpecialProperties(SpecProp, std::end(SpecProp));

using namespace Pylon;
using namespace Basler_UsbCameraParams;
using namespace std;

UnloadCheck::~UnloadCheck() {
	if (gPylonMEXInstance) {
		delete gPylonMEXInstance;
		gPylonMEXInstance = nullptr;
	}
}

BaslerCamera* PylonMEX::camera = nullptr;

PylonMEX::PylonMEX() :
	_initialized(false)
{
	mexPrintf("Create PylonMEX\n");
}


PylonMEX::~PylonMEX()
{
	Shutdown();
	mexPrintf("Deleting PylonMEX\n");
}

void PylonMEX::Initialize() {
	if (_initialized) return;
	try {
		Pylon::PylonInitialize();

		//create camera
		if (!camera) {
			camera = new BaslerCamera;
		}

		//attach camera
		camera->Attach(Pylon::CTlFactory::GetInstance().CreateFirstDevice());

		mexPrintf("Using camera: %s\n", camera->GetDeviceInfo().GetModelName().c_str());

		//configure camera for continuous capture --> may need to change this in the furture to handle complex triggering and sequencing
		camera->RegisterConfiguration(new Pylon::CAcquireContinuousConfiguration, Pylon::RegistrationMode_ReplaceAll, Pylon::Cleanup_Delete);

		camera->MaxNumBuffer = 50;

		//open camera device
		camera->Open();

		//force pixelformat to mono8
		camera->PixelFormat.SetValue(PixelFormat_Mono8);

		//build properties list
		InitializeProperties();

		//set decimation and binning to 1
		if (GenApi::IsAvailable(camera->DecimationVertical) && GenApi::IsWritable(camera->DecimationVertical) ) {
			camera->DecimationVertical.SetValue(1);
		}
		if (GenApi::IsAvailable(camera->DecimationHorizontal) && GenApi::IsWritable(camera->DecimationHorizontal)) {
			camera->DecimationHorizontal.SetValue(1);
		}
		if (GenApi::IsAvailable(camera->BinningHorizontal) && GenApi::IsWritable(camera->BinningHorizontal)) {
			camera->BinningHorizontal.SetValue(1);
		}
		if (GenApi::IsAvailable(camera->BinningVertical) && GenApi::IsWritable(camera->BinningVertical)) {
			camera->BinningVertical.SetValue(1);
		}
		
		//set link speed to maximum
		if (GenApi::IsAvailable(camera->DeviceLinkSelector)) {
			camera->DeviceLinkSelector.SetValue(0);
		}
		if (GenApi::IsAvailable(camera->DeviceLinkThroughputLimit)) {
			camera->DeviceLinkThroughputLimit.SetValue(camera->DeviceLinkThroughputLimit.GetMax());
		}

		//start Grabbing
		camera->StartGrabbing(Pylon::GrabStrategy_LatestImageOnly, GrabLoop_ProvidedByUser);

		//initialize start time
		Pylon::CGrabResultPtr ptrGrabResult;
		camera->RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);
		initDateNum = Matlab_now();
		firstTimeStamp = ptrGrabResult->GetTimeStamp();

		//set init flag
		_initialized = true;
	}
	catch (const GenericException &e)
	{
		mexPrintf("Something went wrong during PylonMEX::Initialize()\n");
		Shutdown(); //shutdown
		throw; //rethrow error
	}
}

void PylonMEX::Shutdown() {
	mexPrintf("PylonMEX::Shutdown\n");
	_initialized = false;

	if (camera) {
		camera->StopGrabbing();
		camera->Close();
		camera->DetachDevice();
		camera->DestroyDevice();
		delete camera;
		camera = nullptr;
	}
	
	Pylon::PylonTerminate();
}

//starts grabbing loop
//just an alias for camera->StartGrabbing(Pylon::GrabStrategy_LatestImageOnly, GrabLoop_ProvidedByUser);
void PylonMEX::StartGrabbing() {
	if(!camera->IsGrabbing())
		camera->StartGrabbing(Pylon::GrabStrategy_LatestImageOnly, GrabLoop_ProvidedByUser);
}

//Convert Grab result timestamp into matlab datenum
//uses initDateNum and firstTimeStamp to determine when camera was first started
double PylonMEX::SnapTimestamp(uint64_t TimeStamp) {
	double dT = double(TimeStamp - firstTimeStamp);
	dT /= 1e9; //convert from ns to sec
	dT /= 86400;//convert to days
	return dT + initDateNum;
	
	
}

//Grab a single image
//Time out after timeoutMS (in millisec)
//Will throw error on timeout
mxArray* PylonMEX::Grab(double timeoutMS, double* MATLAB_NOW) {
	if (!_initialized) throw RUNTIME_EXCEPTION("Camera is not initialized");

	Pylon::CGrabResultPtr ptrGrabResult;

	//camera->RetrieveResult(timeoutMS, ptrGrabResult, TimeoutHandling_ThrowException);
	camera->RetrieveResult(timeoutMS, ptrGrabResult, TimeoutHandling_ThrowException);

	if (MATLAB_NOW) {
		*MATLAB_NOW = SnapTimestamp(ptrGrabResult->GetTimeStamp());
	}

	mxArray* ret = mxCreateNumericMatrix(0, 0, mxUINT8_CLASS, mxREAL);
	trans_copy(ret, ptrGrabResult);

	

	return ret;
	
}

mxArray* PylonMEX::Grab_nowait(double* MATLAB_NOW) {
	if (!_initialized) throw RUNTIME_EXCEPTION("Camera is not initialized");
	mxArray* ret = mxCreateNumericMatrix(0, 0, mxUINT8_CLASS, mxREAL);
	Pylon::CGrabResultPtr ptrGrabResult;
	if (camera->RetrieveResult(0, ptrGrabResult, TimeoutHandling_Return)) {
		if (MATLAB_NOW) {
			*MATLAB_NOW = SnapTimestamp(ptrGrabResult->GetTimeStamp());
		}

		//successful grab
		trans_copy(ret, ptrGrabResult);

	}
	return ret;
}

bool PylonMEX::GrabN(int N,mxArray** pImages, mxArray** pTimes, int* Ngrabbed, double timeoutMS) {
	CDisplayWindowEventHandler * pWindHandler = new CDisplayWindowEventHandler;
	if (N > 1) {
		pWindHandler = new CDisplayWindowEventHandler;
		camera->RegisterImageEventHandler(pWindHandler, Pylon::RegistrationMode_Append, Pylon::Cleanup_Delete);
	}

	*pImages = mxCreateCellMatrix(N, 1);
	if (pTimes) {
		*pTimes = mxCreateDoubleMatrix(N, 1, mxREAL); //datenum format
	}
	*Ngrabbed = N;
	bool grabbedOK = true;
	Pylon::CGrabResultPtr ptrGrabResult;
	for (int n = 0; n < N; ++n) {
		grabbedOK = camera->RetrieveResult(timeoutMS, ptrGrabResult, TimeoutHandling_Return);
		if (!grabbedOK) {
			*Ngrabbed = n;
			break;
		}
		if (pTimes) {
			mxGetPr(*pTimes)[n] = SnapTimestamp(ptrGrabResult->GetTimeStamp());
		}
		mxArray* img = mxCreateNumericMatrix(0, 0, mxUINT8_CLASS, mxREAL);
		trans_copy(img, ptrGrabResult);
		mxSetCell(*pImages, n, img);
	}
	if (N > 1) {
		camera->DeregisterImageEventHandler(pWindHandler);
	}

	return grabbedOK;
}


size_t PylonMEX::BufferCount() {
	if (!_initialized) throw RUNTIME_EXCEPTION("Camera is not initialized");
	return camera->NumReadyBuffers();
}


//Copy the data from src grab result to mxArray
//transpose data during copy
//NOTE: right it on'y works for uint8 data. need to add switch statement
//to handle other types
void trans_copy(mxArray* dest, const Pylon::CGrabResultPtr& src) {
	void* data = mxMalloc(src->GetImageSize());
	transpose((uint8_t*)src->GetBuffer(), (uint8_t*)data, src->GetHeight(), src->GetWidth());
	mxSetData(dest, data);
	mxSetM(dest, src->GetHeight());
	mxSetN(dest, src->GetWidth());
}

void mex_copy(mxArray* dest, const Pylon::CGrabResultPtr& src) {
	void* data = mxMalloc(src->GetImageSize());
	memcpy(data, src->GetBuffer(), src->GetImageSize());
	mxSetData(dest, data);
	mxSetM(dest, src->GetWidth());
	mxSetN(dest, src->GetHeight());
}

// Camera Parameters
double PylonMEX::ResultingFrameRate() {
	if (GenApi::IsAvailable(camera->ResultingFrameRate)) {
		return camera->ResultingFrameRate.GetValue();
	}
	else {
		return 0;
	}
}

//Populate properties array with available properties
//ignores properties in the SpecialProperties List included at the top of PylonMEX.cpp
void PylonMEX::InitializeProperties() {
	GenApi::INodeMap& nodemap = camera->GetNodeMap();
	GenApi::INode * rootnode = nodemap.GetNode("Root");
	RecursiveBuildProperties(rootnode);
}
//recusrively scan through the camera's node list
//populate the properties array if the node references a valid property
//Skip properties in SpecialProperties list
void PylonMEX::RecursiveBuildProperties(GenApi::INode * node) {
	using namespace GenApi;
	if (node->IsFeature() && node->GetVisibility() != Invisible) {
		gcstring PropName = node->GetName();

		// don't process properties included in SpecialProperties
		for (int n = 0; n < SpecialProperties.size(); ++n) {
			if (PropName == SpecialProperties[n].c_str()) {
				//prop is in the SpecialProperties list
				return;
			}
		}

		CameraProperty thisProp;

		//Made it here, this is not a special property
		//check if node is RO or RW (all others are skipped)
		if (node->GetAccessMode() == RO || node->GetAccessMode() == RW) {
			
			//type -> check if it is a value type
			thisProp.proptype = node->GetPrincipalInterfaceType();
			if (thisProp.proptype == intfIBoolean ||
				thisProp.proptype == intfIEnumeration ||
				thisProp.proptype == intfIInteger||
				thisProp.proptype == intfIFloat ||
				thisProp.proptype == intfIString)
			{
				//name
				thisProp.name = PropName;
				//readonly
				thisProp.readonly = false;
				if (node->GetAccessMode() == RO) {
					thisProp.readonly = true;
				}

				//add to property list
				properties.push_back(thisProp);
			}
		}

		//Process node's children
		NodeList_t sublist;
		node->GetChildren(sublist);
		for (size_t n = 0; n < sublist.size(); ++n) {
			RecursiveBuildProperties(sublist[n]);
		}
	}
}

//Camera Parameters to handle separately
//-----------------------------

//Sensor Properties

size_t PylonMEX::SensorWidth() {
	if (GenApi::IsAvailable(camera->SensorWidth)) {
		return camera->SensorWidth();
	}
	return 0;
}
size_t PylonMEX::SensorHeight() {
	if (GenApi::IsAvailable(camera->SensorHeight)) {
		return camera->SensorHeight();
	}
	return 0;
}
size_t PylonMEX::WidthMax() {
	if (GenApi::IsAvailable(camera->WidthMax)) {
		return camera->WidthMax();
	}
	return 0;
}
size_t PylonMEX::HeightMax() {
	if (GenApi::IsAvailable(camera->HeightMax)) {
		return camera->HeightMax();
	}
	return 0;
}

//Exposure
bool PylonMEX::HasExposure() {
	return GenApi::IsAvailable(camera->ExposureTime);
}
void PylonMEX::GetExposureLimits(double* low, double* high) {
	if (GenApi::IsAvailable(camera->ExposureTime)) {
		*low = camera->ExposureTime.GetMin();
		*high = camera->ExposureTime.GetMax();
	}
	else
	{
		*low = mxGetNaN();
		*high = mxGetNaN();
	}
}
double PylonMEX::GetExposure() {
	if (GenApi::IsAvailable(camera->ExposureTime)) {
		return camera->ExposureTime();
	}
	return mxGetNaN();
}
void PylonMEX::SetExposure(double val) {

	//mexPrintf("SetExposure:%f\n", val);

	if (GenApi::IsAvailable(camera->ExposureTime) && GenApi::IsWritable(camera->ExposureTime)) {
		val = min(val, camera->ExposureTime.GetMax());
		val = max(val, camera->ExposureTime.GetMin());
		if (camera->ExposureTime.HasInc()) {
			val = floor(val / camera->ExposureTime.GetInc())*camera->ExposureTime.GetInc();
		}
		camera->ExposureTime.SetValue(val);
	}
}
bool PylonMEX::HasExposureAuto() {
	return GenApi::IsAvailable(camera->ExposureAuto);
}
bool PylonMEX::GetExposureAuto() {
	if (GenApi::IsAvailable(camera->ExposureAuto)) {
		return camera->ExposureAuto();
	}
	return false;
}
void PylonMEX::SetExposureAuto(bool b) {
	if (GenApi::IsAvailable(camera->ExposureAuto) && GenApi::IsWritable(camera->ExposureAuto)) {
		if (b) {
			camera->ExposureAuto.SetValue(ExposureAuto_Continuous);
		}
		else {
			camera->ExposureAuto.SetValue(ExposureAuto_Off);
		}
	}
}

//checks if property is available and is writable
bool PropWritable(const GenApi::IBase& prop) {
	if (GenApi::IsAvailable(prop)) {
		return GenApi::IsWritable(prop);
	}
	return false;
}
//alias for GenApi::IsAvailable()
bool IsProp(const GenApi::IBase& prop) {
	return GenApi::IsAvailable(prop);
}

//ROI
bool PylonMEX::HasROI() {
	return GenApi::IsAvailable(camera->OffsetX) || GenApi::IsAvailable(camera->OffsetX) || GenApi::IsAvailable(camera->Width) || GenApi::IsAvailable(camera->Height);
}
void PylonMEX::ClearROI() {
	if (!HasROI()) return;
	bool wasgrabbing = camera->IsGrabbing();
	camera->StopGrabbing();
	if (PropWritable(camera->CenterX)) {
		camera->CenterX.SetValue(false);
	}
	if (PropWritable(camera->CenterY)) {
		camera->CenterY.SetValue(false);
	}
	if (PropWritable(camera->OffsetX)) {
		camera->OffsetX.SetValue(0);
	}
	if (PropWritable(camera->OffsetY)) {
		camera->OffsetY.SetValue(0);
	}
	if (PropWritable(camera->Width)) {
		camera->Width.SetValue(camera->Width.GetMax());
	}
	if (PropWritable(camera->Height)) {
		camera->Height.SetValue(camera->Height.GetMax());
	}
	if (wasgrabbing) {
		StartGrabbing();
	}
}
void PylonMEX::SetROI(size_t OffsetX, size_t OffsetY, size_t Width, size_t Height) {
	bool wasgrabbing = camera->IsGrabbing();
	camera->StopGrabbing();

	ClearROI();
	//width,height
	if (PropWritable(camera->Width)) {
		Width = min(Width, camera->Width.GetMax());
		Width = max(Width, camera->Width.GetMin());
		Width = (Width / camera->Width.GetInc())*camera->Width.GetInc();
		camera->Width.SetValue(Width);
	}
	if (PropWritable(camera->Height)) {
		Height = min(Height, camera->Height.GetMax());
		Height = max(Height, camera->Height.GetMin());
		Height = (Height / camera->Height.GetInc())*camera->Height.GetInc();
		camera->Height.SetValue(Height);
	}
	//offset
	if (PropWritable(camera->OffsetX)) {
		OffsetX = min(OffsetX, camera->OffsetX.GetMax());
		OffsetX = max(OffsetX, camera->OffsetX.GetMin());
		OffsetX = (OffsetX / camera->OffsetX.GetInc())*camera->OffsetX.GetInc();
		camera->OffsetX.SetValue(OffsetX);
	}
	if (PropWritable(camera->OffsetY)) {
		OffsetY = min(OffsetY, camera->OffsetY.GetMax());
		OffsetY = max(OffsetY, camera->OffsetY.GetMin());
		OffsetY = (OffsetY / camera->OffsetY.GetInc())*camera->OffsetY.GetInc();
		camera->OffsetY.SetValue(OffsetY);
	}


	if (wasgrabbing) {
		StartGrabbing();
	}
}
void PylonMEX::GetROI(size_t* OffsetX, size_t* OffsetY, size_t* Width, size_t* Height) {
	if(IsProp(camera->OffsetX)) {
		*OffsetX = camera->OffsetX();
	}
	else {
		*OffsetX = mxGetNaN();
	}

	if (IsProp(camera->OffsetY)) {
		*OffsetY = camera->OffsetY();
	}
	else {
		*OffsetY = mxGetNaN();
	}

	if (IsProp(camera->Width)) {
		*Width = camera->Width();
	}
	else {
		*Width = mxGetNaN();
	}

	if (IsProp(camera->Height)) {
		*Height = camera->Height();
	}
	else {
		*Height = mxGetNaN();
	}
}

//Camera Properties handled using GenIcam API
//================================
std::vector<std::string> PylonMEX::PropertyNames() {
	vector<string> names;
	for (int n = 0; n < properties.size(); ++n) {
		names.push_back(properties[n].name.c_str());
	}
	return names;
}
std::vector<bool> PylonMEX::PropertyROStates() {
	vector<bool> ro;
	for (int n = 0; n < properties.size(); ++n) {
		ro.push_back(properties[n].readonly);
	}
	return ro;
}
std::vector<std::string> PylonMEX::PropertyTypes() {
	using namespace GenApi;
	vector<string> types;
	for (int n = 0; n < properties.size(); ++n) {
		switch (properties[n].proptype) {
		case intfIInteger:
			types.push_back("intfIInteger");
			break;
		case intfIFloat:
			types.push_back("intfIFloat");
			break;
		case intfIBoolean:
			types.push_back("intfIBoolean");
			break;
		case intfIEnumeration:
			types.push_back("intfIEnumeration");
			break;
		case intfIString:
			types.push_back("intfIString");
			break;
		default:
			types.push_back("other");
		}
	}
}

//finds the index of name in property list
//if not found returns -1
int PylonMEX::PropIndex(const char* name) {
	int ret;
	for (ret = 0; ret <= properties.size(); ++ret) {
		if (ret == properties.size()) {
			return -1;
		}
		if (properties[ret].name == name) {
			return ret;
		}
	}
}


void PylonMEX::SetProperty(const char* Name, double val) {

	//mexPrintf("value before:%f\n", val);

	using namespace GenApi;

	bool wasgrabbing = camera->IsGrabbing();
	bool restartCam = false;

	int idx = PropIndex(Name);
	if (idx < 0) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Property name not in property list");
	}
	
	INode* node = camera->GetNodeMap().GetNode(properties[idx].name);
	if (!node) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Invalid Property name");
	}

	if (node->GetPrincipalInterfaceType() != intfIFloat) {
		throw RUNTIME_EXCEPTION("Called SetProperty(...,double) but property is not intfIFloat type");
	}

	if (properties[idx].readonly) {
		mexPrintf("Cannot set property: %s, readonly\n", Name);
		return;
	}

	//check if camera needs to be stopped
	if (node->GetAccessMode() != RW) {
		camera->StopGrabbing();
		restartCam = true;

		if (node->GetAccessMode() != RW) {
			//still not rw
			if (wasgrabbing) {
				StartGrabbing();
			}
			mexPrintf("Could not set %s, property was not writable even after stopping camera.\n", Name);
			return;
		}
	}

	//set value
	CFloatPtr pVal(node);

	//mexPrintf("Current Value: %f\n", pVal->GetValue());

	/*if (pVal->HasInc()) {
		//mexPrintf("increment: %f\n", pVal->GetInc());
		val = floor(val / pVal->GetInc())*pVal->GetInc(); //convert to increments;
	}*/
	
	val = min(val, pVal->GetMax());
	val = max(val, pVal->GetMin());

	//mexPrintf("value:%f\n", val);
	pVal->SetValue(val);

	if (wasgrabbing && restartCam) {
		StartGrabbing();
	}
}
void PylonMEX::SetProperty(const char* Name, int64_t val) {
	using namespace GenApi;

	bool wasgrabbing = camera->IsGrabbing();
	bool restartCam = false;

	int idx = PropIndex(Name);
	if (idx < 0) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Property name not in property list");
	}

	INode* node = camera->GetNodeMap().GetNode(properties[idx].name);
	if (!node) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Invalid Property name");
	}

	if (node->GetPrincipalInterfaceType() != intfIInteger) {
		throw RUNTIME_EXCEPTION("Called SetProperty(...,int) but property is not intfIInteger type");
	}

	if (properties[idx].readonly) {
		mexPrintf("Cannot set property: %s, readonly\n", Name);
		return;
	}

	//check if camera needs to be stopped
	if (node->GetAccessMode() != RW) {
		camera->StopGrabbing();
		restartCam = true;

		if (node->GetAccessMode() != RW) {
			//still not rw
			if (wasgrabbing) {
				StartGrabbing();
			}
			mexPrintf("Could not set %s, property was not writable even after stopping camera.\n", Name);
			return;
		}
	}

	//set value
	CIntegerPtr pVal(node);

	val = floor(val / pVal->GetInc())*pVal->GetInc(); //convert to increments;
	val = min(val, pVal->GetMax());
	val = max(val, pVal->GetMin());

	pVal->SetValue(val);

	if (wasgrabbing && restartCam) {
		StartGrabbing();
	}
}
void PylonMEX::SetProperty(const char* Name, const string& val) {
	using namespace GenApi;

	bool wasgrabbing = camera->IsGrabbing();
	bool restartCam = false;

	int idx = PropIndex(Name);
	if (idx < 0) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Property name not in property list");
	}

	INode* node = camera->GetNodeMap().GetNode(properties[idx].name);
	if (!node) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Invalid Property name");
	}

	if (node->GetPrincipalInterfaceType() != intfIString && node->GetPrincipalInterfaceType() != intfIEnumeration) {
		throw RUNTIME_EXCEPTION("Called SetProperty(...,string) but property is neither intfIString nor intfIEnumeration type");
	}

	if (properties[idx].readonly) {
		mexPrintf("Cannot set property: %s, readonly\n", Name);
		return;
	}

	//check if camera needs to be stopped
	if (node->GetAccessMode() != RW) {
		camera->StopGrabbing();
		restartCam = true;

		if (node->GetAccessMode() != RW) {
			//still not rw
			if (wasgrabbing) {
				StartGrabbing();
			}
			mexPrintf("Could not set %s, property was not writable even after stopping camera.\n", Name);
			return;
		}
	}

	//set value
	//string
	if (node->GetPrincipalInterfaceType() == intfIString) {
		CStringPtr pVal(node);
		pVal->SetValue(val.c_str());
	}
	//enumeration
	if (node->GetPrincipalInterfaceType() == intfIEnumeration) {
		CEnumerationPtr pVal(node);
		pVal->FromString(val.c_str());
	}

	if (wasgrabbing && restartCam) {
		StartGrabbing();
	}
}
void PylonMEX::SetProperty(const char* Name, bool val) {
	using namespace GenApi;

	bool wasgrabbing = camera->IsGrabbing();
	bool restartCam = false;

	int idx = PropIndex(Name);
	if (idx < 0) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Property name not in property list");
	}

	INode* node = camera->GetNodeMap().GetNode(properties[idx].name);
	if (!node) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Invalid Property name");
	}

	if (node->GetPrincipalInterfaceType() != intfIBoolean) {
		throw RUNTIME_EXCEPTION("Called SetProperty(...,bool) but property is not intfIBoolean type");
	}

	if (properties[idx].readonly) {
		mexPrintf("Cannot set property: %s, readonly\n", Name);
		return;
	}

	//check if camera needs to be stopped
	if (node->GetAccessMode() != RW) {
		camera->StopGrabbing();
		restartCam = true;

		if (node->GetAccessMode() != RW) {
			//still not rw
			if (wasgrabbing) {
				StartGrabbing();
			}
			mexPrintf("Could not set %s, property was not writable even after stopping camera.\n", Name);
			return;
		}
	}

	//set value
	CBooleanPtr pVal(node);

	pVal->SetValue(val);

	if (wasgrabbing && restartCam) {
		StartGrabbing();
	}
}

void PylonMEX::GetProperty(const char* Name, double* val) {
	using namespace GenApi;
	INode* node = camera->GetNodeMap().GetNode(Name);
	if (!node) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Invalid Property name");
	}

	if (node->GetAccessMode() != RO && node->GetAccessMode() != RW) {
		throw RUNTIME_EXCEPTION("Cannot read property");
	}

	if (node->GetPrincipalInterfaceType() != intfIFloat) {
		throw RUNTIME_EXCEPTION("Called GetProperty(...,double*) but property is not intfIFloat type");
	}

	CFloatPtr pVal(node);
	*val = pVal->GetValue();
}
void PylonMEX::GetProperty(const char* Name, int64_t* val) {
	using namespace GenApi;
	INode* node = camera->GetNodeMap().GetNode(Name);
	if (!node) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Invalid Property name");
	}

	if (node->GetAccessMode() != RO && node->GetAccessMode() != RW) {
		throw RUNTIME_EXCEPTION("Cannot read property");
	}

	if (node->GetPrincipalInterfaceType() != intfIInteger) {
		throw RUNTIME_EXCEPTION("Called GetProperty(...,int*) but property is not intfIInteger type");
	}

	CIntegerPtr pVal(node);
	*val = pVal->GetValue();
}
void PylonMEX::GetProperty(const char* Name, string* val) {
	using namespace GenApi;
	INode* node = camera->GetNodeMap().GetNode(Name);
	if (!node) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Invalid Property name");
	}

	if (node->GetAccessMode() != RO && node->GetAccessMode() != RW) {
		throw RUNTIME_EXCEPTION("Cannot read property");
	}

	if (node->GetPrincipalInterfaceType() != intfIString && node->GetPrincipalInterfaceType() != intfIEnumeration) {
		throw RUNTIME_EXCEPTION("Called GetProperty(...,string*) but property is neither intfIString nor intfIEnumeration type");
	}

	if (node->GetPrincipalInterfaceType() == intfIString) {
		CStringPtr pVal(node);
		*val = pVal->GetValue().c_str();
	}
	//enumeration
	if (node->GetPrincipalInterfaceType() == intfIEnumeration) {
		CEnumerationPtr pVal(node);
		*val = pVal->ToString().c_str();
	}
}
void PylonMEX::GetProperty(const char* Name, bool* val) {
	using namespace GenApi;
	INode* node = camera->GetNodeMap().GetNode(Name);
	if (!node) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Invalid Property name");
	}

	if (node->GetAccessMode() != RO && node->GetAccessMode() != RW) {
		throw RUNTIME_EXCEPTION("Cannot read property");
	}

	if (node->GetPrincipalInterfaceType() != intfIBoolean) {
		throw RUNTIME_EXCEPTION("Called GetProperty(...,bool*) but property is not intfIBoolean type");
	}

	CBooleanPtr pVal(node);
	*val = pVal->GetValue();
}

GenApi::EInterfaceType PylonMEX::PropertyType(const char* name) {
	int idx = PropIndex(name);
	if (idx < 0) {
		throw RUNTIME_EXCEPTION("property not found");
	}
	return properties[idx].proptype;
}

std::string TypeToString(GenApi::EInterfaceType typ) {
	using namespace GenApi;
	std::string ret;

	switch (typ) {
	case intfIInteger:
		ret = "intfIInteger";
		break;
	case intfIFloat:
		ret = "intfIFloat";
		break;
	case intfIBoolean:
		ret = "intfIBoolean";
		break;
	case intfIEnumeration:
		ret = "intfIEnumeration";
		break;
	case intfIString:
		ret = "intfIString";
		break;
	default:
		ret = "other";
	}
	return ret;

}

void PylonMEX::EnumeratePropertyValues(const char* Name, GenApi::StringList_t& opts) {
	using namespace GenApi;
	INode* node = camera->GetNodeMap().GetNode(Name);
	if (!node) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Invalid Property name");
	}
	if (node->GetPrincipalInterfaceType() != intfIEnumeration) {
		throw RUNTIME_EXCEPTION("Called EnumeratePropertyValues(...) but property is not intfIEnumeration type");
	}
	CEnumerationPtr pVal(node);
	pVal->GetSymbolics(opts);
}

void PylonMEX::PropertyLimits(const char* Name, double* low, double* high, double* increment) {
	using namespace GenApi;
	INode* node = camera->GetNodeMap().GetNode(Name);
	if (!node) {
		throw RUNTIME_EXCEPTION("PylonMEX::SetProperty() Invalid Property name");
	}
	if (node->GetPrincipalInterfaceType() != intfIFloat && node->GetPrincipalInterfaceType() != intfIInteger) {
		throw RUNTIME_EXCEPTION("Called PropertyLimits(...) but property is nneither intfIFloat nor intfIInteger type");
	}
	EInterfaceType typ = node->GetPrincipalInterfaceType();
	if (typ == intfIInteger) {
		CIntegerPtr pVal(node);
		*low = pVal->GetMin();
		*high = pVal->GetMax();
		*increment = pVal->GetInc();
	}
	else if (typ == intfIFloat) {
		CFloatPtr pVal(node);
		*low = pVal->GetMin();
		*high = pVal->GetMax();
		if (pVal->HasInc()) {
			*increment = pVal->GetInc();
		}
		else {
			*increment = mxGetNaN();
		}
	}
}