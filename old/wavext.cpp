#pragma warning (disable : 4996)
#pragma warning (disable : 4819)
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "quartz.lib")

#include <tchar.h>
#include <stdio.h>
#include <locale.h>
#include <dshow.h>
#include <windows.h>
#include <shlwapi.h>

IPin *GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir);
void PrintFilters(IFilterGraph *pGraph);
void PrintError(HRESULT hr, TCHAR *file, int line);
void WINAPI FreeMediaType(AM_MEDIA_TYPE& mt);

int _tmain(int argc, _TCHAR* argv[])
{
	_tsetlocale(LC_CTYPE, _T("japanese"));

	if (argc != 2 && argc != 3 && argc != 5 && argc != 6)
	{
		_tprintf(_T("usage : wavext [Channels SamplesPerSec BitsPerSample] <file> [<output file>]\n")
			_T("    Channels        1:mono 2:stereo\n")
			_T("    SamplesPerSec   8000 11025 16000 22050 32000 44100 etc\n")
			_T("    BitsPerSample   8 16\n\n")
			_T("    example)   wavext test.mp3\n")
			_T("               wavext 2 44100 16 test.mp3\n")
			_T("               wavext test.mp3 out.wav\n")
			_T("               wavext 2 44100 16 test.mp3 out.wav\n")
			);
		return 0;
	}

	TCHAR outfilebuf[MAX_PATH];
	const TCHAR *infile = NULL;
	TCHAR *outfile = outfilebuf;
	WORD  nChannels = 0;
    DWORD nSamplesPerSec = 0;
    WORD  wBitsPerSample = 0;
	if (argc == 2 || argc == 3)
	{
		infile = argv[1];
		if (argc == 3) outfile = argv[2];
	}
	else
	{
		nChannels      = (WORD)_ttoi(argv[1]);
		nSamplesPerSec = (DWORD)_ttoi(argv[2]);
		wBitsPerSample = (WORD)_ttoi(argv[3]);
		infile = argv[4];
		if (argc == 6) outfile = argv[5];
	}

	if (argc != 3 && argc != 6) {
		StringCbCopy (outfile, MAX_PATH, infile);
		TCHAR *pExt = PathFindExtension(outfile);
		if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, pExt, -1, _T(".wav"), -1) == CSTR_EQUAL)
		{
			StringCbCopy(pExt, MAX_PATH, _T(".new"));
			pExt += 4;
		}
		StringCbCopy(pExt, MAX_PATH, _T(".wav"));
	}
	_tprintf(_T("%s\n"), outfile);

    HRESULT hr;
    IGraphBuilder *pGraph = NULL;

    CoInitialize(NULL);

    CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
                     IID_IGraphBuilder, (LPVOID *)&pGraph);

    // グラフ自動作成
    hr = pGraph->RenderFile(infile, NULL);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}

	// 音声出力フィルタを探す
    IBaseFilter *pFilter1 = NULL;
	IPin *pPinSnd   = NULL;
	IPin *pPinSrc_O = NULL;
	hr = pGraph->FindFilterByName(L"Default DirectSound Device", &pFilter1);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}
	pPinSnd = GetPin(pFilter1, PINDIR_INPUT);
	if (pPinSnd == NULL)
	{
		_ftprintf(stderr, _T("error : pPinSnd=NULL (%s,%d)\n"), _T(__FILE__), __LINE__);
		return 1;
	}
	hr = pPinSnd->ConnectedTo(&pPinSrc_O);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}
	pPinSnd->Release();

	// サンプリングレートを取得
	AM_MEDIA_TYPE mtsrc;
	hr = pPinSrc_O->ConnectionMediaType(&mtsrc);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}
	if (mtsrc.majortype != MEDIATYPE_Audio || mtsrc.subtype != MEDIASUBTYPE_PCM)
	{
		_ftprintf(stderr, _T("error : illegal mediatype\n"));
		return 1;
	}
	if (nChannels != 0)
	{
		if (nSamplesPerSec > ((WAVEFORMATEX*)(mtsrc.pbFormat))->nSamplesPerSec ||
			nChannels*wBitsPerSample > ((WAVEFORMATEX*)(mtsrc.pbFormat))->nChannels * ((WAVEFORMATEX*)(mtsrc.pbFormat))->wBitsPerSample)
		{
			_ftprintf(stderr, _T("error : bitrate too big (%d %d %d)\n"),
				((WAVEFORMATEX*)(mtsrc.pbFormat))->nChannels,
				((WAVEFORMATEX*)(mtsrc.pbFormat))->nSamplesPerSec,
				((WAVEFORMATEX*)(mtsrc.pbFormat))->wBitsPerSample);
			return 1;
		}
	}
	else
	{
		nChannels      = ((WAVEFORMATEX*)(mtsrc.pbFormat))->nChannels;
		nSamplesPerSec = ((WAVEFORMATEX*)(mtsrc.pbFormat))->nSamplesPerSec;
		wBitsPerSample = ((WAVEFORMATEX*)(mtsrc.pbFormat))->wBitsPerSample;
	}
	FreeMediaType(mtsrc);

	// 音声出力フィルタの接続解除
	hr = pGraph->RemoveFilter(pFilter1);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}
    pFilter1->Release();

	// 映像出力を停止
	hr = pGraph->FindFilterByName(L"Video Renderer", &pFilter1);
	if (SUCCEEDED(hr))
	{
		IPin *pPinVidio_I = GetPin(pFilter1, PINDIR_INPUT);
		IPin *pPinVidio_Src;
		pPinVidio_I->ConnectedTo(&pPinVidio_Src);
		PIN_INFO pininfo;
		pPinVidio_Src->QueryPinInfo(&pininfo);

		pPinVidio_I->Release();
		pGraph->RemoveFilter(pFilter1);
		pFilter1->Release();

		pPinVidio_Src->Release();
		pGraph->RemoveFilter(pininfo.pFilter);
		pininfo.pFilter->Release();
	}
	
	// ACM Wrapper Filter作成
    IBaseFilter *pACMWrapper = NULL;
    CoCreateInstance(CLSID_ACMWrapper, NULL, CLSCTX_INPROC_SERVER,
                     IID_IBaseFilter, (void **) &pACMWrapper);

    // フィルターを追加
    hr = pGraph->AddFilter(pACMWrapper, L"ACM Wrapper");
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}

    // Wave Dest
    const GUID CLSID_WavDest = { 0x3c78b8e2, 0x6c4d, 0x11d1, { 0xad, 0xe2, 0x0, 0x0, 0xf8, 0x75, 0x4b, 0x99 } };

    IBaseFilter  *pWaveDst = NULL;
    hr = CoCreateInstance(CLSID_WavDest, NULL, CLSCTX_INPROC_SERVER,
                     IID_IBaseFilter, (void**)&pWaveDst);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}
    hr = pGraph->AddFilter(pWaveDst, L"Wave Dest");
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}

    // File write
    IBaseFilter *pFileWrite = NULL;
    hr = CoCreateInstance(CLSID_FileWriter, NULL, CLSCTX_INPROC_SERVER,
                     IID_IBaseFilter, (void**)&pFileWrite);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}
    hr = pGraph->AddFilter(pFileWrite, L"File write");
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}
    IFileSinkFilter *pDes = NULL;
    pFileWrite->QueryInterface(IID_IFileSinkFilter, (void**)&pDes);
    hr = pDes->SetFileName(outfile, NULL);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}
    pDes->Release();

    // 接続
    IPin *pPinACM_I = GetPin(pACMWrapper, PINDIR_INPUT);
	if (pPinACM_I == NULL)
	{
		_ftprintf(stderr, _T("error : pPinACM_I=NULL (%s,%d)\n"), _T(__FILE__), __LINE__);
		return 1;
	}
    IPin *pPinACM_O = GetPin(pACMWrapper, PINDIR_OUTPUT);
	if (pPinACM_O == NULL)
	{
		_ftprintf(stderr, _T("error : pPinACM_O=NULL (%s,%d)\n"), _T(__FILE__), __LINE__);
		return 1;
	}
    IPin *pPinWav_I = GetPin(pWaveDst, PINDIR_INPUT);
	if (pPinWav_I == NULL)
	{
		_ftprintf(stderr, _T("error : pPinWav_I=NULL (%s,%d)\n"), _T(__FILE__), __LINE__);
		return 1;
	}
    IPin *pPinWav_O = GetPin(pWaveDst, PINDIR_OUTPUT);
	if (pPinWav_O == NULL)
	{
		_ftprintf(stderr, _T("error : pPinWav_O=NULL (%s,%d)\n"), _T(__FILE__), __LINE__);
		return 1;
	}
    IPin *pPinFile_I = GetPin(pFileWrite, PINDIR_INPUT);
	if (pPinFile_I == NULL)
	{
		_ftprintf(stderr, _T("error : pPinFile_I=NULL (%s,%d)\n"), _T(__FILE__), __LINE__);
		return 1;
	}
    hr = pGraph->Connect(pPinSrc_O, pPinACM_I);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}
    hr = pGraph->Connect(pPinACM_O, pPinWav_I);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}
    hr = pGraph->Connect(pPinWav_O, pPinFile_I);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}
    pPinSrc_O->Release();
    pPinACM_I->Release();
    pPinWav_I->Release();
    pPinWav_O->Release();
    pPinFile_I->Release();

    // サンプリングレート設定(メモ：PIN接続後)
    IAMStreamConfig *pAMStreamCfg = NULL;
    pPinACM_O->QueryInterface(IID_IAMStreamConfig, (void **)&pAMStreamCfg);

    WAVEFORMATEX   wf;
    ZeroMemory(&wf, sizeof(wf));
    wf.nChannels       = nChannels;
    wf.nSamplesPerSec  = nSamplesPerSec;
    wf.wBitsPerSample  = wBitsPerSample;
    wf.nBlockAlign     = wf.nChannels * wf.wBitsPerSample / 8;
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
    wf.wFormatTag      = WAVE_FORMAT_PCM;

    AM_MEDIA_TYPE   mt;
    ZeroMemory(&mt, sizeof(mt));
    mt.majortype         = MEDIATYPE_Audio;
    mt.subtype           = MEDIASUBTYPE_PCM;
    mt.bFixedSizeSamples = TRUE;
    mt.lSampleSize       = 4;
    mt.formattype        = FORMAT_WaveFormatEx;
    mt.cbFormat          = sizeof(WAVEFORMATEX);
    mt.pbFormat          = (BYTE *)&wf;

    hr = pAMStreamCfg->SetFormat(&mt);
    if (FAILED(hr))
    {
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
    }
    pAMStreamCfg->Release();
    pPinACM_O->Release();

    // 表示
#if _DEBUG
    PrintFilters(pGraph);
#endif

	// すでにあるファイルを消す
	DeleteFile(outfile);

    // 再生
    IMediaControl *pMediaControl = NULL;
	pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	hr = pMediaControl->Run();
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}

	// 終了待機
	IMediaEvent *pMediaEvent = NULL;
	long eventCode;
	pGraph->QueryInterface(IID_IMediaEvent, (void **)&pMediaEvent);
	hr = pMediaEvent->WaitForCompletion(-1, &eventCode);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}

    hr = pMediaControl->Stop();
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return 1;
	}

    pACMWrapper->Release();
    pWaveDst->Release();
    pFileWrite->Release();
    pMediaControl->Release();
	pMediaEvent->Release();
    pGraph->Release();

    CoUninitialize();

	return 0;
}

IPin *GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir)
{
	HRESULT hr;
    BOOL       bFound = FALSE;
    IEnumPins  *pEnum;
    IPin       *pPin;

    hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		PrintError(hr, _T(__FILE__), __LINE__);
		return NULL;
	}
    while(pEnum->Next(1, &pPin, 0) == S_OK)
    {
        PIN_DIRECTION PinDirThis;
        pPin->QueryDirection(&PinDirThis);
		if (FAILED(hr))
		{
			PrintError(hr, _T(__FILE__), __LINE__);
			return NULL;
		}
        if (bFound = (PinDir == PinDirThis)) // 引数で指定した方向のピンならbreak
            break;
        pPin->Release();
    }
    pEnum->Release();
    return (bFound ? pPin : NULL);
}

void PrintFilters(IFilterGraph *pGraph)
{
    IEnumFilters *pEnum;
    IBaseFilter *pFilter = NULL;

    pGraph->EnumFilters(&pEnum);
    while(pEnum->Next(1, &pFilter, NULL) == S_OK)
    {
        FILTER_INFO FilterInfo;
        pFilter->QueryFilterInfo(&FilterInfo);

        _tprintf(_T(" < %s"), FilterInfo.achName);
        pFilter->Release();
    }
	_putts(_T("\n"));
    pEnum->Release();
}

void PrintError(HRESULT hr, TCHAR *file, int line)
{
    TCHAR lpBuffer[MAX_ERROR_TEXT_LEN+1];
    AMGetErrorText(hr, lpBuffer, MAX_ERROR_TEXT_LEN);
	_ftprintf(stderr, _T("error : %s (%s,%d)\n"), lpBuffer, file, line);
}

void WINAPI FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0) {
        CoTaskMemFree((PVOID)mt.pbFormat);

        // Strictly unnecessary but tidier
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) {
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}
