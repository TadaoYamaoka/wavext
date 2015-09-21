#pragma warning (disable : 4996)
#pragma warning (disable : 4819)

#include <tchar.h>
#include <dshow.h>
#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>

IPin *GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir);
void PrintFilters(IFilterGraph *pGraph);
void PrintError(HRESULT hr);

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 2 && argc != 5)
	{
		wprintf(_T("usage : wavconv [Channels SamplesPerSec BitsPerSample] <file>\n")
			_T("    Channels        1:mono(default) 2:stereo\n")
			_T("    SamplesPerSec   8000 11025(default) 16000 22050 32000 44100 etc\n")
			_T("    BitsPerSample   8(default) 16\n\n")
			_T("    remark)  Channels,SamplesPerSec,BitsPerSample :\n")
			_T("               if dest > source then abort.\n\n")
			_T("    example)   wavconv test.mp3\n")
			_T("               wavconv 2 44100 16 test.mp3\n"));
		return 0;
	}

	const TCHAR *infile;
	WORD  nChannels      = 1;
    DWORD nSamplesPerSec = 11025;
    WORD  wBitsPerSample = 8;
	if (argc == 2)
	{
		infile = argv[1];
	}
	else
	{
		nChannels      = (WORD)_ttoi(argv[1]);
		nSamplesPerSec = (DWORD)_ttoi(argv[2]);
		wBitsPerSample = (WORD)_ttoi(argv[3]);
		infile = argv[4];
	}

	TCHAR outfile[MAX_PATH];
	StringCbCopy (outfile, MAX_PATH, infile);
	TCHAR *pExt = PathFindExtension(outfile);
	StringCbCopy (pExt, MAX_PATH, _T(".wav"));
	wprintf(_T("%s\n"), outfile);

    HRESULT hr;
    IGraphBuilder *pGraph;
    IMediaControl *pMediaControl;

    CoInitialize(NULL);

    CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
                     IID_IGraphBuilder, (LPVOID *)&pGraph);

    // グラフ自動作成
    hr = pGraph->RenderFile(infile, NULL);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}

    IEnumFilters *pEnum;
    IBaseFilter *pFilter1 = NULL;
    IBaseFilter *pFilter2 = NULL;

    pGraph->EnumFilters(&pEnum);
    pEnum->Next(1, &pFilter1, NULL);
    pEnum->Next(1, &pFilter2, NULL);

    pGraph->RemoveFilter(pFilter1);
    pFilter1->Release();
    IPin *pPinSrc_O = GetPin(pFilter2, PINDIR_OUTPUT);
    pFilter2->Release();

    // ACM Wrapper Filter作成
    IBaseFilter *pACMWrapper;
    CoCreateInstance(CLSID_ACMWrapper, NULL, CLSCTX_INPROC_SERVER,
                     IID_IBaseFilter, (void **) &pACMWrapper);

    // フィルターを追加
    pGraph->AddFilter(pACMWrapper, L"ACM Wrapper");

    // Wave Dest
    const GUID CLSID_WavDest = { 0x3c78b8e2, 0x6c4d, 0x11d1, { 0xad, 0xe2, 0x0, 0x0, 0xf8, 0x75, 0x4b, 0x99 } };

    IBaseFilter  *pWaveDst;
    CoCreateInstance(CLSID_WavDest, NULL, CLSCTX_INPROC_SERVER,
                     IID_IBaseFilter, (void**)&pWaveDst);
    pGraph->AddFilter(pWaveDst, L"Wave Dest");

    // File write
    IBaseFilter *pFileWrite;
    CoCreateInstance(CLSID_FileWriter, NULL, CLSCTX_INPROC_SERVER,
                     IID_IBaseFilter, (void**)&pFileWrite);
    pGraph->AddFilter(pFileWrite, L"File write");

    IFileSinkFilter *pDes;
    pFileWrite->QueryInterface(IID_IFileSinkFilter, (void**)&pDes);
    pDes->SetFileName(outfile, NULL);
    pDes->Release();

    // 接続
    IPin *pPinACM_I = GetPin(pACMWrapper, PINDIR_INPUT);
    IPin *pPin = GetPin(pACMWrapper, PINDIR_OUTPUT);
    IPin *pPinWav_I = GetPin(pWaveDst, PINDIR_INPUT);
    IPin *pPinWav_O = GetPin(pWaveDst, PINDIR_OUTPUT);
    IPin *pPinFile_I = GetPin(pFileWrite, PINDIR_INPUT);
    pGraph->Connect(pPinSrc_O, pPinACM_I);
    pGraph->Connect(pPin, pPinWav_I);
    pGraph->Connect(pPinWav_O, pPinFile_I);
    pPinSrc_O->Release();
    pPinACM_I->Release();
    pPin->Release();
    pPinWav_I->Release();
    pPinWav_O->Release();
    pPinFile_I->Release();

    // サンプリングレート設定(メモ：PIN接続後)
    IAMStreamConfig *pAMStreamCfg;
    pPin->QueryInterface(IID_IAMStreamConfig, (void **)&pAMStreamCfg);

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
		PrintError(hr);
    }
    pAMStreamCfg->Release();
    pPin->Release();

    // 表示
    PrintFilters(pGraph);

    // 再生
	pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
	pMediaControl->Run();

	// 終了待機
	IMediaEvent *pMediaEvent;
	long eventCode;
	pGraph->QueryInterface(IID_IMediaEvent, (void **)&pMediaEvent);
	pMediaEvent->WaitForCompletion(-1, &eventCode);
	pMediaEvent->Release();
    //MessageBox(NULL, _T("Click me to end playback."), _T("DirectShow"), MB_OK);

    pMediaControl->Stop();

    pACMWrapper->Release();
    pWaveDst->Release();
    pFileWrite->Release();
    pMediaControl->Release();
    pGraph->Release();

    CoUninitialize();

	return 0;
}

IPin *GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir)
{
    BOOL       bFound = FALSE;
    IEnumPins  *pEnum;
    IPin       *pPin;

    pFilter->EnumPins(&pEnum);
    while(pEnum->Next(1, &pPin, 0) == S_OK)
    {
        PIN_DIRECTION PinDirThis;
        pPin->QueryDirection(&PinDirThis);
        if (bFound = (PinDir == PinDirThis)) // 引数で指定した方向のピンならbreak
            break;
        pPin->Release();
    }
    pEnum->Release();
    return (bFound ? pPin : 0);
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

        wprintf(_T(" < %s"), FilterInfo.achName);
        pFilter->Release();
    }
	wprintf(_T("\n"));
    pEnum->Release();
}

void PrintError(HRESULT hr)
{
    TCHAR lpBuffer[MAX_ERROR_TEXT_LEN+1];
    AMGetErrorText(hr, lpBuffer, MAX_ERROR_TEXT_LEN);
    MessageBox(NULL, lpBuffer, _T("error message"), MB_ICONHAND|MB_OK);
}
