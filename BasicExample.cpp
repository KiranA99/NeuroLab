#include <windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <fstream>

// contains templated, easy to use functions for controlling the
// functions exposed through AmplifierSDK.h
#include "SDK.h"

// note: this header is already supplied by SDK.h
// it is explicitly re-included for completeness' sake
#include "AmplifierSDK.h"

// class for handling raw amplifer data
#include "../RawDataHandlerExample.h"

#include <random>

#define SENT 4

int Number_of_files = 6;
std::vector<std::pair<std::string, std::string>> m_vpAmpDetails;
static CAmplifier amp;
CRITICAL_SECTION m_CriticalSection;
bool m_bIsThreadRunning;

// to demonstrate channel labeling options
std::string sChannelLabels_10_20_32 = "Fp1,Fp2,F7,F3,Fz,F4,F8,FC5,FC1,FC2,FC6,T7,C3,Cz,C4,T8,TP9,CP5,CP1,CP2,CP6,TP10,P7,P3,Pz,P4,P8,PO9,O1,Oz,O2,PO10";

// forward declaration needed for some functions:
void CheckImpedances(std::vector<float>& vfImpVals);
DWORD __stdcall RecordFunc(void* pVoid);
void Set1020();

// synchronized function for writing data to file
DWORD WINAPI PrintFunc(void* pVoid)
{

	RawDataHandler rdh(amp);
	int nSamples;

	LARGE_INTEGER liTimeInfo;
	INT64 nCounterStart;
	QueryPerformanceFrequency(&liTimeInfo);
	double dPCFrequency = double(liTimeInfo.QuadPart) / 1000;
	QueryPerformanceCounter(&liTimeInfo);
	nCounterStart = liTimeInfo.QuadPart;
	double dTime;
	//double dTime = lsl::local_clock();
	std::vector<std::vector<float>> vvfData;
	amp.StartAcquisition(RM_NORMAL);
	InitializeCriticalSection(&m_CriticalSection);
	while (m_bIsThreadRunning)
	{
		EnterCriticalSection(&m_CriticalSection);
		
		nSamples = rdh.ParseRawData(amp, vvfData);
		QueryPerformanceCounter(&liTimeInfo);
		dTime = double(liTimeInfo.QuadPart - nCounterStart) / dPCFrequency;
		if (nSamples != 0)
			std::cout << nSamples << " samples: in " << dTime << " milliseconds\n";
		else
		{
			Sleep((DWORD)1);
			QueryPerformanceCounter(&liTimeInfo);
			nCounterStart = liTimeInfo.QuadPart;
		}
		LeaveCriticalSection(&m_CriticalSection);
	}
	DeleteCriticalSection(&m_CriticalSection);
	amp.StopAcquisition();
	return TRUE;
}
void PrintChannelValues()
{

	RawDataHandler rdh(amp);
	HANDLE hThread = NULL;
	std::vector<std::vector<float>> vvfData;
	int nChoice = -1;
	while (nChoice!=1)
	{

		std::cout << "\n\nChoose from available options:";
		std::cout << "\n\t0: One shot data grab";
		std::cout << "\n\t1: Go back to main menu";
		std::cout << "\n\t2: Print Loop";

		std::cout << "\n>> ";
		std::cin >> nChoice;
		int nSamples = 0;
		// first count the number of enabled channels
		int nEnabled = 0;
		int nEnabledCnt = 0;
		int nAvailableChannels;
		int nRes = amp.GetProperty(nAvailableChannels, DevicePropertyID::DPROP_I32_AvailableChannels);
		for (int i = 0; i < nAvailableChannels; i++)
		{
			nRes = amp.GetProperty(nEnabled, i, ChannelPropertyID::CPROP_B32_RecordingEnabled);
			if (nEnabled == 1)
				nEnabledCnt++;
		}
		if (nChoice == 0)
		{
			amp.StartAcquisition(RM_NORMAL);
			while (nSamples == 0)
				nSamples = rdh.ParseRawData(amp, vvfData);
			std::cout << "Received " << nSamples << "samples from " << nEnabledCnt << "channels:\n";
			for (int i = 0; i < nEnabledCnt; i++)
			{
				std::cout << "Ch[" << i << "] ";
				for (int j = 0; j < nSamples; j++)
				{
					std::cout << "S[" << j << "]: ";
					std::cout << vvfData[j][i];
				}
				std::cout << "\n";
			}
			amp.StopAcquisition();
		}
		if (nChoice == 2)
		{
			m_bIsThreadRunning = true;
			hThread = CreateThread(NULL, 0, PrintFunc, NULL, 0, NULL);
			if (hThread == 0)
			{
				std::cout << "Error spawning recording thread\n";
				return;
			}
		}

		
	}
	// kill the thread
	if (m_bIsThreadRunning == true)
	{
		m_bIsThreadRunning = false;
		WaitForSingleObject(hThread, INFINITE);
		std::cout << "\nDone\n";
	}

}

struct RecordingParams{
	int nSampleSize;
	float fSR;
	int32_t nBufferLen;
};

// synchronized function for writing data to file
DWORD WINAPI RecordFunc(void* pVoid) // Writing Data function
{

	RecordingParams pRecordingParams = *(RecordingParams*)pVoid;
	int nLast = 0;
	std::vector<BYTE> Buffer;
	int res = SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	Buffer.resize(pRecordingParams.nBufferLen);
	InitializeCriticalSection(&m_CriticalSection);
	while (m_bIsThreadRunning)
	{
		try
		{
			int nRet = AMP_OK;
			int nLSLRetVal = 0;
			int nRecRetVal = 0;
			EnterCriticalSection(&m_CriticalSection);
			nRet = amp.GetData(&Buffer[0], (int)Buffer.size(), (int)Buffer.size());
			//add number of samples


			if (nRet > 0)
				nRecRetVal = CStorage::StoreDataBlock(amp.m_hAmplifier, &Buffer[0], nRet); // store the internal memory data

			
			
	

			LeaveCriticalSection(&m_CriticalSection);

		}
		catch (const std::exception&)
		{
			LeaveCriticalSection(&m_CriticalSection);
			return FALSE;
		}

	}



	DeleteCriticalSection(&m_CriticalSection);
	return TRUE;
}



string IntToStr(int n) {
	stringstream result;
	result << n;
	return result.str();

}

void Record(){



//--------------REDO---------------\\

//std::string sFileName;
HANDLE hThread = NULL;
m_bIsThreadRunning = false;
int nSecondsCtr = 0;
int option;




// get the necessary information from the amplifier
RawDataHandler rdh(amp);
int nSampleSize = rdh.getSampleSize();
float fBaseSR;
float fSubSampleDevisor;
int nRes = amp.GetProperty(fBaseSR, DPROP_F32_BaseSampleRate);
if (nRes != AMP_OK)
{
	// handle error
	return;
}
nRes = amp.GetProperty(fSubSampleDevisor, DPROP_F32_SubSampleDivisor);
if (nRes != AMP_OK)
{
	// handle error
	return;
}

// pack the information
RecordingParams pRecordingParams;
pRecordingParams.fSR = fBaseSR / fSubSampleDevisor;
pRecordingParams.nSampleSize = nSampleSize;
pRecordingParams.nBufferLen = (int)(pRecordingParams.fSR * .2 * 200);


//std::cout << "\n\nPlease enter filename, e.g. \'rec01.eeg\' (.dat, .vmrk, and .vhdr files will be written to this directory):\n" << ">> ";
//std::cin >> sFileName;


/*for (int i = 0; i < Number_of_files; i++) {
	sFileName = "data_" + IntToStr(i) + ".eeg";
	printf("\n");
	cout << sFileName << " \n";
}
	std::cout << "\nUse 10-20 channel labels (assumes 32 channels)?\n\t0: no\n\t1: yes\n>> ";
	int nAddChannelLabels = 0;
	std::cin >> nAddChannelLabels;
	if (nAddChannelLabels != 0)
		Set1020();

	//initialize the recording---the final argument is whether or not to record to the SD card in the case of LiveAmp
	nRes = CStorage::StartRecording(amp, ("./" + sFileName).c_str(), "optional comment", false);
	if (nRes != AMP_OK)
	{
		//handle error
		//return;
	}*/

	// start data flow
	nRes = amp.StartAcquisition(RecordingMode::RM_NORMAL);
	if (nRes != AMP_OK)
	{
		// handle error
		return;
	}

	// launch recording thread
	m_bIsThreadRunning = true;
	hThread = CreateThread(NULL, 0, RecordFunc, &pRecordingParams, 0, NULL);
	if (hThread == 0)
	{
		std::cout << "Error spawning recording thread\n";
		return;
	}
	
		// wait for about 10 seconds
		while (nSecondsCtr < 4.98)
		{
			Sleep(1000);
			std::cout << "\n\t...writing...";
			nSecondsCtr++;

		}


	// kill the thread
	if (m_bIsThreadRunning == true)
	{
		m_bIsThreadRunning = false;
		WaitForSingleObject(hThread, INFINITE);
	}
	std::cout << "\nDone\n";

	// stop data flow
	nRes = amp.StopAcquisition();
	if (nRes != AMP_OK) {
		// handle error
		return;
	}

	

}

void Set1020()
{
	CStorage::SetChannelLabels(amp.m_hAmplifier, sChannelLabels_10_20_32.c_str());
}

void CheckImpedances(std::vector<float>& vfImpVals)
{
	BOOL bChannelSupportImp;
	int nRet;
	int nAvailableChannels;
	int nImpChns = 0;
	std::vector<float> vfImpData;
	
	amp.GetProperty(nAvailableChannels, DPROP_I32_AvailableChannels);
	for (int i = 0; i < nAvailableChannels; i++)
	{
		nRet = amp.GetProperty(bChannelSupportImp, i, CPROP_B32_ImpedanceMeasurement);
		if (!bChannelSupportImp || nRet < 0)
			continue;

		nImpChns++;
	}

	if (nImpChns == 0)
	{
		std::cout << "\nNo impedance data available. Are any electrodes attached?";
		return;
	}
	// this is how to gather impedance data:
	vfImpVals.resize(nImpChns, -1.0);
	// there are 2*nChns + 1 values of raw data to parse 
	nImpChns = 2*(nImpChns + 1);
	vfImpData.resize(nImpChns, -1.0);
	std::cout << "\n\nImpedance data: ";
	int nCnt = 0;
	amp.StartAcquisition(RM_IMPEDANCE);
	nRet = -1; 
	std::string sAmpFamily;
	int nIsRef = 0;
	while(nRet<=AMP_OK)
		nRet = amp.GetData(&vfImpData[0], vfImpData.size() * sizeof(float), vfImpData.size() * sizeof(float)); //edit this 
	float fVal;
	if (nRet > AMP_OK)
	{
		// note that for actiCHamp, a reference channel must be chosen 
		// in LiveAmp, there is a dedicated reference electrode
		fVal = vfImpData[0];
		std::cout << "\nGnd: " << fVal;
		fVal = vfImpData[1];
		std::cout << "\nRef: " << fVal;
		for (std::vector<float>::iterator it = vfImpData.begin()+2;
			it != vfImpData.end();
			it += 2)
		{
			fVal = (*(it + 1) < 0) ? *it : *it - *(it + 1); // this is because electrodes may be bipolar
			std::cout << "\nCh_" << nCnt << ": " << fVal;
			nCnt++;

		}
	}
	amp.StopAcquisition();

}

// example for how to change a device property
void ChangeSampleRate(){
	
	float fSR, fSSD;
	int nIdx;
	int nRes;

	// SDK structures for property ranges (templated)
	PropertyRange<float> prAvailableSampleRates;
	PropertyRange<float> prAvailableSubSampleDivisors;
	// get the available values for this property
	nRes = amp.GetPropertyRange(prAvailableSampleRates, DPROP_F32_BaseSampleRate);
	if (nRes != AMP_OK)
	{
		std::cout << "\nERROR in GetPropertyRange BaseSampleRate:\t" << nRes;
		return;
	}
	// get the available values for this property
	nRes = amp.GetPropertyRange(prAvailableSubSampleDivisors, DPROP_F32_SubSampleDivisor);
	if (nRes != AMP_OK)
	{
		std::cout << "\nERROR in GetPropertyRange SubSampleDivisor:\t" << nRes;
		return;
	}
	bool bHasSSD = true;
	if (prAvailableSubSampleDivisors.ByteLength == 0)
	{
		bHasSSD = false;
		fSSD = 1.0;
	}
	// queery the current sampling rates
	nRes = amp.GetProperty(fSR, DPROP_F32_BaseSampleRate);
	if (nRes != AMP_OK)
	{
		std::cout << "\nERROR in GetProperty BaseSampleDivisor:\t" << nRes;
		return;
	}
	std::cout << "\n\tCurrent Base Sample Rate: " << fSR;
	amp.GetProperty(fSSD, DPROP_F32_SubSampleDivisor);
	if (nRes != AMP_OK)
	{
		std::cout << "\nERROR in GetProperty SubSampleDivisor:\t" << nRes;
		return;
	}
	if (bHasSSD)std::cout << "\n\tCurrent Sub-Sample Divisor: " << fSSD;
	std::cout << "\n\tCurrent Effective Sampling Rate: " << (int)(fSR / fSSD);

	nRes = amp.GetProperty(fSR, DPROP_F32_BaseSampleRate);
	if (nRes != AMP_OK)
	{
		std::cout << "\nERROR in GetProperty BaseSampleRate:\t" << nRes;
		return;
	}

	int nAvailable = prAvailableSampleRates.ByteLength / sizeof(float);
	std::cout << "\n\nChoose from available Base Sample Rates:";
	for (int i = 0; i < nAvailable; i++)
		std::cout << "\n\t" << i << ": " << prAvailableSampleRates.RangeArray[i] << " ";
	std::cout << "\n>> ";
	std::cin >> nIdx;

	nRes = amp.SetProperty(prAvailableSampleRates.RangeArray[nIdx], DPROP_F32_BaseSampleRate);
	if (nRes != AMP_OK)
	{
		std::cout << "\nERROR in SetProperty BaseSampleRate:\t" << nRes;
		return;
	}

	nRes = amp.GetProperty(fSR, DPROP_F32_BaseSampleRate);
	if (nRes != AMP_OK)
	{
		std::cout << "\nERROR in GetProperty BaseSampleRate:\t" << nRes;
		return;
	}
	std::cout << "\n\tCurrent Base Sample Rate: " << fSR;

	if (bHasSSD)
	{
		nRes = amp.GetProperty(fSSD, DPROP_F32_SubSampleDivisor);
		if (nRes != AMP_OK)
		{
			std::cout << "\nERROR in GetProperty SubSampleDivisor:\t" << nRes;
			return;
		}
		nAvailable = prAvailableSubSampleDivisors.ByteLength / sizeof(float);
		std::cout << "\n\nChoose from available Sub-Sample Divisors:";
		for (int i = 0; i < nAvailable; i++)
			std::cout << "\n\t" << i << ": " << prAvailableSubSampleDivisors.RangeArray[i] << " ";
		std::cout << "\n>> ";
		std::cin >> nIdx;
		nRes = amp.SetProperty(prAvailableSubSampleDivisors.RangeArray[nIdx], DPROP_F32_SubSampleDivisor);
		if (nRes != AMP_OK)
		{
			std::cout << "\nERROR in SetProperty SubSampleDivisor:\t" << nRes;
			return;
		}

		nRes = amp.GetProperty(fSSD, DPROP_F32_SubSampleDivisor);
		if (nRes != AMP_OK)
		{
			std::cout << "\nERROR in GetProperty SubSampleDivisor:\t" << nRes;
			return;
		}
		std::cout << "\n\tCurrent Sub-Sample Divisor: " << fSSD;
	}
	std::cout << "\n\tCurrent Effective Sampling Rate: " << (int)(fSR / fSSD);
	
}

void DisplaySomeProperties()
{
	std::string sProp;
	int nProp;
	float fProp;
	int nRes;
	t_VersionNumber tvnProp;

	nRes = amp.StartAcquisition(RM_NORMAL);

	// queery device a few properties
	std::cout << "\n\nSome Device Properties (for a complete list see Amplifier_LIB.h):";
	amp.GetProperty(tvnProp, DPROP_TVN_HardwareRevision);
	std::cout << "\n\tHardwareRevision:\t" << tvnProp.Major << "." <<
		tvnProp.Minor << "." << tvnProp.Build << "." << tvnProp.Revision;
	amp.GetProperty(nProp, DPROP_I32_AvailableModules);
	std::cout << "\n\tAvailableModuls:\t" << nProp;

	// a few module properties
	std::cout << "\n\nSome Module Properties for module 0:";
	amp.GetProperty(sProp, 0, MPROP_CHR_Type);
	std::cout << "\n\tType:\t" << sProp;
	amp.GetProperty(nProp, 0, MPROP_I32_UseableChannels);
	std::cout << "\n\tUsableChannels:\t" << nProp;
	std::cout << "\n\nSome Channel Properties for channel 0:";

	// a few channel properties
	amp.GetProperty(nProp, 0, CPROP_I32_ChannelNumber);
	std::cout << "\n\tChannelNumber:\t" << nProp ;
	amp.GetProperty(fProp, 0, CPROP_F32_Resolution);
	std::cout << "\n\tResolution:\t" << fProp;

	amp.StopAcquisition();
	
}

void ConnectToAmp(int nIdx)
{
	

	amp.Close();
	amp.m_hAmplifier = NULL;
	int nRes = amp.Open(nIdx);
	if (nRes != AMP_OK)
	{
		std::cout << "\nERROR in opening amplifier:\t" << nRes;
		return;
	}
	//Retrieve Versions 
	VersionNumber apiVersion, libVersion;
	CDllHandler::GetInfo(InfoType::eAPIVersion, (void*)&apiVersion, sizeof(t_VersionNumber));
	CDllHandler::GetInfo(InfoType::eLIBVersion, (void*)&libVersion, sizeof(t_VersionNumber));
	std::cout << "\n\nConnected to: " << m_vpAmpDetails[nIdx].first << " " << m_vpAmpDetails[nIdx].second << ": ";
	std::cout <<"\n\tAPI Version " <<
		apiVersion.Major << "." <<
		apiVersion.Minor << "." <<
		apiVersion.Build << "." <<
		apiVersion.Revision;

	std::cout << "\n\tLibrary Version " <<
		libVersion.Major << "."  <<
		libVersion.Minor << "." <<
		libVersion.Build << "." <<
		libVersion.Revision;

}

int DisplayAmpInfo(int nCount)
{
	std::string sSerialNumber;
	std::string sType;
	std::pair<std::string, std::string> pssAmpDetails;
	CAmplifier amp;
	m_vpAmpDetails.clear();
	int nRes;
	for (int i = 0; i < nCount; i++)
	{
		// open an amplifier using CAmplifier class from SDK.h
		// then query the device for serial number and device type
		nRes = amp.Open(i);
		if (nRes != AMP_OK)
		{
			std::cout << "\nERROR in opening amplifier:\t" << nRes;
			return nRes;
		}
		// DevicePropertyID is enumerated in Amplifier_LIB.h
		nRes = amp.GetProperty(sSerialNumber, DevicePropertyID::DPROP_CHR_SerialNumber);
		if (nRes != AMP_OK)
		{
			std::cout << "\nERROR in GetProperty SerialNumber:\t" << nRes;
			return nRes;
		}
		nRes = amp.GetProperty(sType, DevicePropertyID::DPROP_CHR_Type);
		if (nRes != AMP_OK)
		{
			std::cout << "\nERROR in GetProperty Type:\t" << nRes;
			return nRes;
		}
		nRes = amp.Close();
		if (nRes != AMP_OK)
		{
			std::cout << "\nERROR in closing amplifier:\t" << nRes;
			return nRes;
		}
		pssAmpDetails.first = sType;
		pssAmpDetails.second = sSerialNumber;
		// print the results and store the details for later
 		m_vpAmpDetails.push_back(pssAmpDetails);
		std::cout << "\n\nSelect from available devices:\n" <<
			"\t" << i << ": " << sType << " " << sSerialNumber;
	}
	int nIdx;
	std::cout << "\n>> ";
	std::cin >> nIdx;
	return nIdx;
}

int SearchForAmps(){

	std::cout << " Welcome To AntiChamp DEVICE\n" "Searching for devices...";
	

	// error codes enumerated in Amplifier_LIB.h
	int nRes;

	// container for Device address
	std::string sHWDeviceAddress = ""; //5002 100010-0374

	// container for interface type
	char hwi[20];
	strcpy_s(hwi, "USB");

	// AmplifierSDK call to enumerate connected devices:
	nRes = EnumerateDevices(hwi, sizeof(hwi), (const char*)sHWDeviceAddress.data(), 0);
 	return nRes;
}

int SelectAmpFamily()
{
	int nRes;

	// AmplifierSDK call to select amp family:
	nRes = SetAmplifierFamily((eActiChampFamily));
	return nRes;
}

int main()
{
	std::string sFileName;
	std::cout << "Welcome to a basic Brain Products SDK:\n\n\n";
	//"Please follow the instructions below.";
	int nRes;
	amp.Close();
	nRes = SelectAmpFamily();
	nRes = SearchForAmps();
	if (nRes < 1)
	{
		std::cout << "\n\nCould not find any amplifier according to the above parameters.";
		std::cout << "\n\tEnter 'x' to exit:";
		std::cout << "\n>> ";
		std::cin >> nRes;
		return  0;
	}
	nRes = DisplayAmpInfo(nRes);
	
	int option;

	ConnectToAmp(nRes);
	DisplaySomeProperties();

	std::vector<float> vfImpVals;
	
	ChangeSampleRate();
	
	for (int i = 0; i < Number_of_files; i++) {
		sFileName = "data_" + IntToStr(i) + ".eeg";
		printf("\n");
		cout << sFileName << " \n";
		
		
		std::cout << "\nUse 10-20 channel labels (assumes 32 channels)?\n\t0: no\n\t1: yes\n>> ";
		int nAddChannelLabels = 0;
		std::cin >> nAddChannelLabels;
		if (nAddChannelLabels != 0)
			Set1020();
		
		Record();

		//initialize the recording---the final argument is whether or not to record to the SD card in the case of LiveAmp
		nRes = CStorage::StartRecording(amp, ("./" + sFileName).c_str(), "optional comment", false);
		//if (nRes != AMP_OK)
		//{
			//handle error
			//return;
		//}
	
	}
		
		amp.Close();
		return 0;
}


