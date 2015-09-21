#include <DShow.h>
#include <windows.h>
#include <stdio.h>

void PrintFilters(IFilterGraph *pGraph);
IPin *GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir);

void main()
{
    HRESULT hr;
    IGraphBuilder *pGraph;
    IMediaControl *pMediaControl;

    CoInitialize(NULL);

    CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
                     IID_IGraphBuilder, (LPVOID *)&pGraph);

    // 自動作成
    pGraph->RenderFile(L"in.mp3", NULL);

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
    pDes->SetFileName(L"out.wav", NULL);
    pDes->Release();

    // 接続
    //pGraph->Disconnect(pPin);
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
    wf.nChannels       = 1;
    wf.nSamplesPerSec  = 11025;
    wf.wBitsPerSample  = 8;
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
        TCHAR lpBuffer[MAX_ERROR_TEXT_LEN];
        AMGetErrorText(hr, lpBuffer, MAX_ERROR_TEXT_LEN);
        MessageBox(NULL, lpBuffer, "error message", MB_ICONHAND|MB_OK);
    }
    pAMStreamCfg->Release();
    pPin->Release();

    // 表示
    PrintFilters(pGraph);

    // 再生
    pGraph->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
    pMediaControl->Run();

    MessageBox(NULL,"Click me to end playback.","DirectShow",MB_OK);

    pMediaControl->Stop();

    pACMWrapper->Release();
    pWaveDst->Release();
    pFileWrite->Release();
    pMediaControl->Release();
    pGraph->Release();

    CoUninitialize();
}


void PrintFilters(IFilterGraph *pGraph)
{
    IEnumFilters *pEnum;
    IBaseFilter *pFilter = NULL;

    HRESULT hr;
    pGraph->EnumFilters(&pEnum);
    while(pEnum->Next(1, &pFilter, NULL) == S_OK)
    {
        FILTER_INFO FilterInfo;
        pFilter->QueryFilterInfo(&FilterInfo);

        char buffer[MAX_FILTER_NAME*2];
        WideCharToMultiByte(CP_ACP, 0, FilterInfo.achName, -1, buffer, sizeof(buffer), NULL, NULL);
        printf("%s\n", buffer);
        pFilter->Release();
    }
    pEnum->Release();
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

