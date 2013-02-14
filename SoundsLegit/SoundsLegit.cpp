// SoundsLegit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define CINTERFACE 1
#define COBJMACROS 1

#include <dsound.h>

#include <initguid.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <Audioclient.h>

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C,
	    0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35,
	    0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1,0x78, 0xc2,0xf5,0x68,0xa7,0x03,0xb2);
DEFINE_GUID(IID_IAudioCaptureClient, 0xc8adbd64, 0xe71e, 0x48a0, 0xa4,0xde, 0x18,0x5c,0x39,0x5c,0xd3,0x17);


_TCHAR sndOut[] = _T("Speakers and Headphones (IDT High Definition Audio CODEC)");

LPGUID target = NULL;

BOOL CALLBACK EnumDev(LPGUID lpGUID, LPCTSTR lpszDesc, LPCTSTR lpszDrvName, LPVOID lpContext)
{
	_tprintf(_T("\t [%s] [%s]\n"), lpszDrvName, lpszDesc);
	/*if (_tcscmp(sndOut, lpszDesc) == 0)
	{
	printf("!!!!!!!!!!!!!!!!\n");
	target = lpGUID;
	}*/
	return TRUE;
}

int dsound_capture()
{
	FILE * pFile;
	HRESULT hr;
	IDirectSoundCapture8 * cap;

	IDirectSoundCaptureBuffer8* capBuf;
	DSCBUFFERDESC dscbd;
	LPDIRECTSOUNDCAPTUREBUFFER  pDSCB;
	WAVEFORMATEX wfx = {WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0};

	pFile = fopen("dsound_capture.raw", "wb");

	//need to look at all the capture devices
	hr = DirectSoundEnumerate(EnumDev,NULL);

	if (FAILED(hr))
	{
		_tprintf(_T("Failed to get sound devices\n"));
		return 0;
	}

	/*if (target == NULL)
	{
	_tprintf(_T("Failed to find device\n"));
	return 0;
	}*/

	hr = DirectSoundCaptureCreate8(target, &cap, NULL);

	if (FAILED(hr))
	{
		_tprintf(_T("Failed to create sound capture device\n"));
		return 0;
	}
	_tprintf(_T("Created sound capture device\n"));



	// wFormatTag, nChannels, nSamplesPerSec, mAvgBytesPerSec,
	// nBlockAlign, wBitsPerSample, cbSize

	dscbd.dwSize = sizeof(DSCBUFFERDESC);
	dscbd.dwFlags = 0;
	dscbd.dwBufferBytes = wfx.nAvgBytesPerSec * 10;
	dscbd.dwReserved = 0;
	dscbd.lpwfxFormat = &wfx;
	dscbd.dwFXCount = 0;
	dscbd.lpDSCFXDesc = NULL;

	hr = cap->lpVtbl->CreateCaptureBuffer(cap, &dscbd, &pDSCB, NULL);

	if (FAILED(hr))
	{
		_tprintf(_T("Failed to create capture buffer\n"));
	}
	_tprintf(_T("Created capture buffer"));


	hr = pDSCB->lpVtbl->QueryInterface(pDSCB, &IID_IDirectSoundCaptureBuffer8, (LPVOID*)&capBuf);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to QI capture buffer\n"));
	}
	_tprintf(_T("Created IDirectSoundCaptureBuffer8\n"));
	pDSCB->lpVtbl->Release(pDSCB); 


	_tprintf(_T("Trying to start capture\n"));
	hr = capBuf->lpVtbl->Start(capBuf, DSCBSTART_LOOPING);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to start capture\n"));
	}
	_tprintf(_T("Capture started\n"));

	Sleep(10000);

	{
		VOID* pbCaptureData  = NULL;
		DWORD dwCaptureLength;
		VOID* pbCaptureData2 = NULL;
		DWORD dwCaptureLength2;
		VOID* pbPlayData   = NULL;
		DWORD dwReadPos;
		LONG lLockSize;

		hr = capBuf->lpVtbl->GetCurrentPosition(capBuf, NULL, &dwReadPos);
		if (FAILED(hr))
		{
			_tprintf(_T("Failed\n"));
			return 0;
		}

		lLockSize = dscbd.dwBufferBytes;

		hr = capBuf->lpVtbl->Lock(capBuf, 0L, lLockSize, &pbCaptureData, &dwCaptureLength, &pbCaptureData2, &dwCaptureLength2, 0L);
		if (FAILED(hr))
		{
			_tprintf(_T("Failed\n"));
			return 0;
		}

		fwrite(pbCaptureData, 1, dwCaptureLength, pFile);
		fwrite(pbCaptureData2, 1, dwCaptureLength2, pFile);

		hr = capBuf->lpVtbl->Unlock(capBuf, pbCaptureData, dwCaptureLength, pbCaptureData2, dwCaptureLength2);
		if (FAILED(hr))
		{
			_tprintf(_T("Failed\n"));
			return 0;
		}

	}


	_tprintf(_T("Trying to stop capture\n"));
	hr = capBuf->lpVtbl->Stop(capBuf);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to stop capture\n"));
	}
	_tprintf(_T("Capture stopped\n"));

	fclose(pFile);

	return 0;
}

int get_matching_device_string(LPWSTR pattern, LPWSTR * deviceStr)
{

	HRESULT hr;
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDeviceCollection *pCollection = NULL;
	IMMDevice *pEndpoint = NULL;
	IPropertyStore *pProps = NULL;
	LPWSTR pwszID = NULL;
	unsigned int count, i;

	_tprintf(_T("First lets try enumerating the audio capture devices...\n"));
	CoInitialize(NULL);
	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void **) &pEnumerator);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to cocreate device enumerator\n"));
		exit(1);
	}

	hr = pEnumerator->lpVtbl->EnumAudioEndpoints(pEnumerator, eCapture, DEVICE_STATE_ACTIVE, &pCollection);
	if ( FAILED(hr) )
	{
		_tprintf(_T("Failed to create endpoint collection\n"));
		exit(1);
	}

	pCollection->lpVtbl->GetCount(pCollection, &count);
	_tprintf(_T("Num endpoints: %d\n"), count);

	if (count == 0)
	{
		_tprintf(_T("No endpoints!\n"));
		exit(1);
	}

	for (i = 0; i < count; ++i)
	{
		PROPVARIANT nameVar;
		PropVariantInit(&nameVar);

		hr = pCollection->lpVtbl->Item(pCollection, i, &pEndpoint);
		if ( FAILED(hr) )
		{
			_tprintf(_T("Failed to get endpoint %d\n"), i);
			exit(1);
		}


		hr = pEndpoint->lpVtbl->GetId(pEndpoint, &pwszID);
		if ( FAILED(hr) )
		{
			_tprintf(_T("Failed to get endpoint ID\n"));
			exit(1);
		}



		hr = pEndpoint->lpVtbl->OpenPropertyStore(pEndpoint, STGM_READ, &pProps);
		if ( FAILED(hr) )
		{
			_tprintf(_T("Failed to open property store\n"));
			exit(1);
		}


		hr = pProps->lpVtbl->GetValue(pProps, &PKEY_Device_FriendlyName, &nameVar);
		if ( FAILED(hr) )
		{
			_tprintf(_T("Failed to get device friendly name\n"));
			exit(1);
		}

		_tprintf(_T("Endpoint %d: [%s] (%s)\n"), i, nameVar.pwszVal, pwszID);

		//do this a more reliable way
		if (wcscmp(pattern, nameVar.pwszVal) < 0)
		{
			unsigned int devStrLen;
			_tprintf(_T("matched %d characters\n"), wcscmp(pattern, nameVar.pwszVal));

			devStrLen = wcslen(pwszID);
			*deviceStr = (LPWSTR) malloc((devStrLen * 2) + 2);
			ZeroMemory(*deviceStr, (devStrLen * 2) + 2);
			wcscpy_s(*deviceStr, devStrLen+1, pwszID);
		}
		CoTaskMemFree(pwszID);
		pwszID = NULL;
		PropVariantClear(&nameVar);

		pProps->lpVtbl->Release(pProps);
		pProps = NULL;

		pEndpoint->lpVtbl->Release(pEndpoint);
		pEndpoint = NULL;

	}

	pCollection->lpVtbl->Release(pCollection);
	pCollection = NULL;

	pEnumerator->lpVtbl->Release(pEnumerator);
	pEnumerator = NULL;

	return 0;
}

int record_audio(LPWSTR deviceStr)
{
	FILE * pFile;
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice *pDevice = NULL;
	IAudioClient *pAudioClient = NULL;
	IAudioCaptureClient *pCaptureClient = NULL;
	WAVEFORMATEX *pwfx = NULL;
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	REFERENCE_TIME hnsActualDuration;
	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	UINT32 packetLength = 0;
	UINT32 dCount = 0;
	BYTE *pData;

	pFile = fopen("wasapi_capture.raw", "wb");

	CoInitialize(NULL);
	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void **) &pEnumerator);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to cocreate device enumerator\n"));
		exit(1);
	}

	hr = pEnumerator->lpVtbl->GetDevice(pEnumerator, deviceStr, &pDevice);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to cocreate get device\n"));
		exit(1);
	}

	hr = pDevice->lpVtbl->Activate(pDevice, &IID_IAudioClient, CLSCTX_ALL, NULL, (void **)&pAudioClient);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to activate audio client\n"));
		exit(1);
	}

	hr = pAudioClient->lpVtbl->GetMixFormat(pAudioClient, &pwfx);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to get mix format\n"));
		exit(1);
	}

	//screw it, use our own format
	//WAVEFORMATEX wfx = {WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0};
	pwfx->wFormatTag = WAVE_FORMAT_PCM;
	pwfx->nChannels = 2;
	pwfx->nSamplesPerSec = 44100;
	pwfx->nAvgBytesPerSec = 176400;
	pwfx->nBlockAlign = 4;
	pwfx->wBitsPerSample = 16;
	pwfx->cbSize = 0;

	hr = pAudioClient->lpVtbl->Initialize(
		pAudioClient,
		AUDCLNT_SHAREMODE_SHARED,
		0,
		hnsRequestedDuration,
		0,
		pwfx,
		NULL);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to initialize the audio client\n"));
		exit(1);
	}

	hr = pAudioClient->lpVtbl->GetBufferSize(pAudioClient, &bufferFrameCount);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to get buffer size\n"));
		exit(1);
	}

	hr = pAudioClient->lpVtbl->GetService(pAudioClient, &IID_IAudioCaptureClient, (void **) &pCaptureClient);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to get the capture client\n"));
		exit(1);
	}


	hnsActualDuration = (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;

	hr = pAudioClient->lpVtbl->Start(pAudioClient);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to start capture\n"));
		exit(1);
	}

	dCount = 0;
	while(dCount++ < 10)
	{
		DWORD flags;


		Sleep(hnsActualDuration/REFTIMES_PER_MILLISEC/2);

		hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
		if (FAILED(hr))
		{
			_tprintf(_T("Failed to get packet length\n"));
			exit(1);
		}

		while(packetLength != 0)
		{
			hr = pCaptureClient->lpVtbl->GetBuffer(pCaptureClient, &pData, &numFramesAvailable, &flags, NULL, NULL);
			if (FAILED(hr))
			{
				_tprintf(_T("Failed to get buffer\n"));
				exit(1);
			}

			//write data here
			fwrite(pData, 1, packetLength * 4, pFile);

			hr = pCaptureClient->lpVtbl->ReleaseBuffer(pCaptureClient, numFramesAvailable);
			if (FAILED(hr))
			{
				_tprintf(_T("Failed to release buffer\n"));
				exit(1);
			}

			hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
			if (FAILED(hr))
			{
				_tprintf(_T("Failed to get packet length\n"));
				exit(1);
			}
		}
	}

	pAudioClient->lpVtbl->Stop(pAudioClient);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to stop audio client\n"));
		exit(1);
	}

	CoTaskMemFree(pwfx);

	if (pEnumerator != NULL)
		pEnumerator->lpVtbl->Release(pEnumerator);

	if (pDevice != NULL)
		pDevice->lpVtbl->Release(pDevice);

	if (pAudioClient != NULL)
		pAudioClient->lpVtbl->Release(pAudioClient);

	if (pCaptureClient != NULL)
		pCaptureClient->lpVtbl->Release(pCaptureClient);

	fclose(pFile);

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	wchar_t * pattern = L"Stereo Mix";
	LPWSTR dev = NULL;


	_tprintf(_T("Testing direct sound...\n"));

	dsound_capture();

	_tprintf(_T("Testing wasapi...\n"));
	//get device string of "stereo mix"
	get_matching_device_string(pattern, &dev);
	//start recording with that device
	record_audio(dev);
	return 0;
}
