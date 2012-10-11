// SoundsLegit.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <dsound.h>

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


int _tmain(int argc, _TCHAR* argv[])
{
	FILE * pFile;
	HRESULT hr;
	IDirectSoundCapture8 * cap;

	pFile = fopen("capture.raw", "wb");

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


	IDirectSoundCaptureBuffer8* capBuf;
	DSCBUFFERDESC dscbd;
	LPDIRECTSOUNDCAPTUREBUFFER  pDSCB;
	WAVEFORMATEX wfx = {WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0};
	// wFormatTag, nChannels, nSamplesPerSec, mAvgBytesPerSec,
	// nBlockAlign, wBitsPerSample, cbSize

	dscbd.dwSize = sizeof(DSCBUFFERDESC);
	dscbd.dwFlags = 0;
	dscbd.dwBufferBytes = wfx.nAvgBytesPerSec * 10;
	dscbd.dwReserved = 0;
	dscbd.lpwfxFormat = &wfx;
	dscbd.dwFXCount = 0;
	dscbd.lpDSCFXDesc = NULL;

	hr = cap->CreateCaptureBuffer(&dscbd, &pDSCB, NULL);

	if (FAILED(hr))
	{
		_tprintf(_T("Failed to create capture buffer\n"));
	}
	_tprintf(_T("Created capture buffer"));


	hr = pDSCB->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&capBuf);
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to QI capture buffer\n"));
	}
	_tprintf(_T("Created IDirectSoundCaptureBuffer8\n"));
	pDSCB->Release(); 


	_tprintf(_T("Trying to start capture\n"));
	hr = capBuf->Start(DSCBSTART_LOOPING);
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
		UINT  dwDataWrote;
		DWORD dwReadPos;
		LONG lLockSize;

		hr = capBuf->GetCurrentPosition(NULL, &dwReadPos);
		if (FAILED(hr))
		{
			_tprintf(_T("Failed\n"));
			return 0;
		}

		lLockSize = dscbd.dwBufferBytes;

		hr = capBuf->Lock( 0L, lLockSize, &pbCaptureData, &dwCaptureLength, &pbCaptureData2, &dwCaptureLength2, 0L);
		if (FAILED(hr))
		{
			_tprintf(_T("Failed\n"));
			return 0;
		}

		fwrite(pbCaptureData, 1, dwCaptureLength, pFile);
		fwrite(pbCaptureData2, 1, dwCaptureLength2, pFile);

		hr = capBuf->Unlock(pbCaptureData, dwCaptureLength, pbCaptureData2, dwCaptureLength2);
		if (FAILED(hr))
		{
			_tprintf(_T("Failed\n"));
			return 0;
		}

	}


	_tprintf(_T("Trying to stop capture\n"));
	hr = capBuf->Stop();
	if (FAILED(hr))
	{
		_tprintf(_T("Failed to stop capture\n"));
	}
	_tprintf(_T("Capture stopped\n"));

	fclose(pFile);
	
	return 0;
}

