#pragma once
#include <mex.h>

//BaslerIncludes
#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>
#include <pylon/PylonIncludes.h>
#include <pylon/PylonGUI.h> //pylon generated windows

#include <string>

#include "MatrixTranspose.h"
#include "MatlabTime.h"

void trans_copy(mxArray* dest, const Pylon::CGrabResultPtr& src);
void mex_copy(mxArray* dest, const Pylon::CGrabResultPtr& src);

#ifdef BUILD_DLL
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif

typedef Pylon::CBaslerUsbInstantCamera BaslerCamera;

class mexBufferFactory : public Pylon::IBufferFactory {
protected:
	unsigned long m_lastBufferContext;
public:
	mexBufferFactory(): m_lastBufferContext(1000)
	{
	}

	virtual ~mexBufferFactory()
	{
	}

	// Will be called when the Instant Camera object needs to allocate a buffer.
	// Return the buffer and context data in the output parameters.
	// In case of an error new() will throw an exception
	// which will be forwarded to the caller to indicate an error.
	// Warning: This method can be called by different threads.
	virtual void AllocateBuffer(size_t bufferSize, void** pCreatedBuffer, intptr_t& bufferContext)
	{
		try
		{
			// Allocate buffer for pixel data.
			// If you already have a buffer allocated by your image processing library you can use this instead.
			// In this case you must modify the delete code (see below) accordingly.

			*pCreatedBuffer = mxMalloc(bufferSize);

			// The context information is never changed by the Instant Camera and can be used
			// by the buffer factory to manage the buffers.
			// The context information can be retrieved from a grab result by calling
			// ptrGrabResult->GetBufferContext();
			bufferContext = ++m_lastBufferContext;

		}
		catch (const std::exception&)
		{
			// In case of an error we must free the memory we may have already allocated.
			if (*pCreatedBuffer != NULL)
			{
				mxFree(*pCreatedBuffer);
				*pCreatedBuffer = NULL;
			}

			// Rethrow exception.
			// AllocateBuffer can also just return with *pCreatedBuffer = NULL to indicate
			// that no buffer is available at the moment.
			throw;
		}
	}

	// Frees a previously allocated buffer.
	// Warning: This method can be called by different threads.
	virtual void FreeBuffer(void* pCreatedBuffer, intptr_t bufferContext)
	{
		mxFree(pCreatedBuffer);
		pCreatedBuffer = NULL;
	}

	// Destroys the buffer factory.
	// This will be used when you pass the ownership of the buffer factory instance to pylon
	// by defining Cleanup_Delete. pylon will call this function to destroy the instance
	// of the buffer factory. If you don't pass the ownership to pylon (Cleanup_None)
	// this method will be ignored.
	virtual void DestroyBufferFactory()
	{
		delete this;
	}
};

struct CameraProperty {
	GenICam::gcstring name;
	bool readonly;
	GenApi::EInterfaceType proptype;
};

class CDisplayWindowEventHandler : public Pylon::CImageEventHandler
{
public:
	virtual void OnImageGrabbed(Pylon::CInstantCamera& camera, const Pylon::CGrabResultPtr& ptrGrabResult)
	{
	
#ifdef PYLON_WIN_BUILD
		// Display the image
		Pylon::DisplayImage(1, ptrGrabResult);
#endif
	}

};

class API PylonMEX
{
private:
	//Pylon Interface
	//Pylon::PylonAutoInitTerm autoTerm;
	static BaslerCamera* camera;
	bool _initialized;

	void StartGrabbing();
	

	//Camera Properties
	std::vector<CameraProperty> properties;
	void InitializeProperties();
	void RecursiveBuildProperties(GenApi::INode * node);

	int PropIndex(const char* name);

	//timestamp
	double initDateNum;
	uint64_t firstTimeStamp;
public:
	PylonMEX();
	~PylonMEX();

	void Initialize();
	void Shutdown();
	mxArray* Grab(double timeoutMS = 5000, double* MATLAB_NOW = nullptr);
	mxArray* Grab_nowait(double* MATLAB_NOW = nullptr);
	bool GrabN(int N, mxArray** pImages,mxArray** TimesArray, int* Ngrabbed, double timeoutMS = 5000);
	size_t BufferCount();
	//mxArray* Grab(size_t n, double timeoutMS = 5000);

	double SnapTimestamp(uint64_t TickStamp);

	//Camera Parameters to handle separately
	//-----------------------------

	//Sensor Properties
	size_t SensorWidth();
	size_t SensorHeight();
	size_t WidthMax();
	size_t HeightMax();


	//FrameRate
	double ResultingFrameRate();

	//Exposure
	bool HasExposure();
	void GetExposureLimits(double* low, double* high);
	double GetExposure();
	void SetExposure(double val);
	bool HasExposureAuto();
	bool GetExposureAuto();
	void SetExposureAuto(bool b);

	//ROI
	bool HasROI();
	void ClearROI();
	void SetROI(size_t OffsetX, size_t OffsetY, size_t Width, size_t Height);
	void GetROI(size_t* OffsetX, size_t* OffsetY, size_t* Width, size_t* Height);

	//Camera Properties handled using GenIcam API
	//================================
	std::vector<std::string> PropertyNames();
	std::vector<bool> PropertyROStates();
	std::vector<std::string> PropertyTypes();

	void SetProperty(const char* Name, double val);
	void SetProperty(const char* Name, int64_t val);
	void SetProperty(const char* Name, const std::string& val);
	void SetProperty(const char* Name, bool val);

	void GetProperty(const char* Name, double* val);
	void GetProperty(const char* Name, int64_t* val);
	void GetProperty(const char* Name, std::string* val);
	void GetProperty(const char* Name, bool* val);

	void EnumeratePropertyValues(const char* Name, GenApi::StringList_t& opts);
	void PropertyLimits(const char* Name, double* low, double* high, double* increment);

	GenApi::EInterfaceType PropertyType(const char* name);
	

};

API extern PylonMEX* gPylonMEXInstance;

class UnloadCheck {
	~UnloadCheck();
};

API std::string TypeToString(GenApi::EInterfaceType typ);