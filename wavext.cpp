#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "quartz.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shlwapi.lib")
#ifdef _DEBUG
#pragma comment(lib, "strmbasd.lib")
#else
#pragma comment(lib, "strmbase.lib")
#endif // _DEBUG

#include <streams.h>
#include <strsafe.h>
#include <locale.h>
#include <shlwapi.h>

#ifdef _DEBUG
#define TRACE(X, ...) DEBUG_PRINTF(X, __VA_ARGS__)
void DEBUG_PRINTF(const wchar_t *format, ...)
{
	wchar_t buf[1024];
	va_list arg_list;
	va_start(arg_list, format);
	StringCbVPrintf(buf, sizeof(buf), format, arg_list);
	va_end(arg_list);
	OutputDebugString(buf);
}
#else
#define TRACE(X, ...)
#endif // _DEBUG

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)
#define PrintError(hr) PrintError_Imp(hr, __WFILE__, __LINE__)


void PrintError_Imp(HRESULT hr, wchar_t *file, int line);
void PrintFilters(IFilterGraph *pGraph);
void PrintGUID(REFGUID rguid);
HRESULT GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
void PrintUsage();


#pragma pack(push, 1)
struct RiffHeader {
	char          id[4];
	unsigned long size;
	char          fmt[4];
};

struct FormatChunk {
	char           chunkID[4];
	long           chunkSize;
	short          wFormatTag;
	unsigned short wChannels;
	unsigned long  dwSamplesPerSec;
	unsigned long  dwAvgBytesPerSec;
	unsigned short wBlockAlign;
	unsigned short wBitsPerSample;
	/* Note: there may be additional fields here, depending upon wFormatTag. */
};

struct DataChunk {
	char           chunkID[4];
	long           chunkSize; 
};
#pragma pack(pop)


// {090FAFD5-A410-4244-8993-999D378717D0}
static const GUID CSLID_RENDERER_TEST = {0x90fafd5, 0xa410, 0x4244, {0x89, 0x93, 0x99, 0x9d, 0x37, 0x87, 0x17, 0xd0}};


class CRendererDst : public CBaseRenderer
{
public:
	CRendererDst(wchar_t *strFile, HRESULT *phr) : CBaseRenderer(CSLID_RENDERER_TEST, L"Renderer Dst", NULL, phr), hFile(NULL), data_size(0)
	{
		StringCbCopy(outfile, sizeof(outfile), strFile);

		// ファイルオープン
		hFile = CreateFile(outfile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			*phr = GetLastError();
		}
		else
		{
			*phr = S_OK;
		}
	}

	~CRendererDst() {}

	HRESULT CheckMediaType(const CMediaType *pmt)
	{
		if (IsEqualGUID(pmt->majortype, MEDIATYPE_Audio)
			&& IsEqualGUID(pmt->subtype, MEDIASUBTYPE_PCM))
		{
			return S_OK;
		}
		else
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	HRESULT DoRenderSample(IMediaSample *pMediaSample)
	{
		LONG size = pMediaSample->GetActualDataLength();

		BYTE *pBuffer;
		pMediaSample->GetPointer(&pBuffer);

		DWORD written;
		BOOL ret = WriteFile(hFile, pBuffer, size, &written, NULL);
		if (ret == FALSE)
		{
			PrintError(GetLastError());
			CloseHandle(hFile);
			return E_FAIL;
		}

		data_size += size;

		return S_OK;
	}

	HRESULT OnStartStreaming()
	{
		HRESULT hr;

		IPin *pPin;
		FindPin(L"In", &pPin);
		AM_MEDIA_TYPE mt;
		hr = pPin->ConnectionMediaType(&mt);
		if (FAILED(hr))
		{
			PrintError(hr);
			return E_FAIL;
		}

		WORD nChannels      = ((WAVEFORMATEX*)(mt.pbFormat))->nChannels;
		DWORD nSamplesPerSec = ((WAVEFORMATEX*)(mt.pbFormat))->nSamplesPerSec;
		WORD wBitsPerSample = ((WAVEFORMATEX*)(mt.pbFormat))->wBitsPerSample;

		FreeMediaType(mt);

		// RIFFヘッダー
		struct RiffHeader header;

		CopyMemory(&header.id, "RIFF", 4);
		CopyMemory(&header.fmt, "WAVE", 4);
		header.size = sizeof(struct RiffHeader) - 8 + sizeof(struct FormatChunk) + sizeof(struct DataChunk) + data_size;

		// fmtチャンク
		struct FormatChunk fmt;
		CopyMemory(&fmt.chunkID, "fmt ", 4);
		fmt.chunkSize = sizeof(struct FormatChunk) - 8;
		fmt.wFormatTag = 1;
		fmt.wChannels = nChannels;
		fmt.dwSamplesPerSec = nSamplesPerSec;
		fmt.wBlockAlign = nChannels * wBitsPerSample / 8;
		fmt.wBitsPerSample = wBitsPerSample;
		fmt.dwAvgBytesPerSec = nSamplesPerSec * fmt.wBlockAlign;

		// dataチャンク
		struct DataChunk dchunk;
		CopyMemory(&dchunk.chunkID, "data", 4);
		dchunk.chunkSize = data_size;

		BOOL ret;
		DWORD written;

		ret = WriteFile(hFile, &header, sizeof(header), &written, NULL);
		if (ret == FALSE)
		{
			PrintError(GetLastError());
			CloseHandle(hFile);
			return E_FAIL;
		}

		ret = WriteFile(hFile, &fmt, sizeof(fmt), &written, NULL);
		if (ret == FALSE)
		{
			PrintError(GetLastError());
			CloseHandle(hFile);
			return E_FAIL;
		}

		ret = WriteFile(hFile, &dchunk, sizeof(dchunk), &written, NULL);
		if (ret == FALSE)
		{
			PrintError(GetLastError());
			CloseHandle(hFile);
			return E_FAIL;
		}

		return S_OK;
	}

	HRESULT OnStopStreaming()
	{
		BOOL ret;
		DWORD written;

		unsigned long header_size = sizeof(struct RiffHeader) - 8 + sizeof(struct FormatChunk) + sizeof(struct DataChunk) + data_size;
		SetFilePointer(hFile, 4, NULL, FILE_BEGIN);
		ret = WriteFile(hFile, &header_size, sizeof(header_size), &written, NULL);
		if (ret == FALSE)
		{
			PrintError(GetLastError());
			CloseHandle(hFile);
			return E_FAIL;
		}

		const size_t chunkSizePos = sizeof(struct RiffHeader) + sizeof(struct FormatChunk) + sizeof(struct DataChunk) - sizeof(long);
		SetFilePointer(hFile, chunkSizePos, NULL, FILE_BEGIN);
		ret = WriteFile(hFile, &data_size, sizeof(data_size), &written, NULL);
		if (ret == FALSE)
		{
			PrintError(GetLastError());
			CloseHandle(hFile);
			return E_FAIL;
		}

		CloseHandle(hFile);

		return S_OK;
	}

private:
	HANDLE hFile;
	wchar_t outfile[MAX_PATH];
	long data_size;
};

int wmain(int argc, wchar_t* argv[])
{
	setlocale(LC_ALL, ".OCP");

	if (argc != 1 && argc != 2 && argc != 3 && argc != 5 && argc != 6)
	{
		PrintUsage();
		return 0;
	}

	wchar_t infilebuf[MAX_PATH] = {0};
	wchar_t outfilebuf[MAX_PATH];
	const wchar_t *infile;
	wchar_t *outfile = outfilebuf;
	WORD nChannels = 0;
    DWORD nSamplesPerSec = 0;
    WORD wBitsPerSample = 0;

	if (argc == 1)
	{
		wchar_t strFileTitle[MAX_PATH] = {0};

		OPENFILENAME ofn = {0};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL;
		ofn.lpstrFilter = L"Audio files(*.wav;*.mp3;*.wma)\0*.wav;*.mp3;*.wma\0All files(*.*)\0*.*\0\0";
		ofn.lpstrFile = infilebuf;
		ofn.lpstrFileTitle = strFileTitle;
		ofn.nMaxFile = MAX_PATH;
		ofn.nMaxFileTitle = MAX_PATH;
		ofn.Flags = OFN_FILEMUSTEXIST;
		ofn.lpstrTitle = L"ファイルを開く";
		ofn.lpstrDefExt = NULL;

		if (GetOpenFileName(&ofn) == 0)
		{
			PrintUsage();
			return 0;
		}

		infile = infilebuf;
	}
	else if (argc == 2 || argc == 3)
	{
		infile = argv[1];
		if (argc == 3) outfile = argv[2];
	}
	else
	{
		nChannels      = (WORD)_wtoi(argv[1]);
		nSamplesPerSec = (DWORD)_wtoi(argv[2]);
		wBitsPerSample = (WORD)_wtoi(argv[3]);
		infile = argv[4];
		if (argc == 6) outfile = argv[5];
	}

	if (argc != 3 && argc != 6) {
		StringCbCopy (outfile, MAX_PATH, infile);
		TCHAR *pExt = PathFindExtension(outfile);
		if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, pExt, -1, L".wav", -1) == CSTR_EQUAL)
		{
			StringCbCopy(pExt, MAX_PATH, L".new");
			pExt += 4;
		}
		StringCbCopy(pExt, MAX_PATH, L".wav");
	}
	wprintf(L"%s\n", outfile);

	HRESULT hr;

	CoInitialize(NULL);

    // グラフ自動作成
	IGraphBuilder *pGraph = NULL;
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (LPVOID *)&pGraph);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}
    hr = pGraph->RenderFile(infile, NULL);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}

	// 音声出力フィルタを探す
    IBaseFilter *pFilter1 = NULL;
	IPin *pPinSnd   = NULL;
	IPin *pPinSrc_O = NULL;
	hr = pGraph->FindFilterByName(L"Default DirectSound Device", &pFilter1);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}
	hr = GetPin(pFilter1, PINDIR_INPUT, &pPinSnd);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}
	hr = pPinSnd->ConnectedTo(&pPinSrc_O);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}
	pPinSnd->Release();

	// サンプリングレートを取得
	AM_MEDIA_TYPE mtsrc;
	hr = pPinSrc_O->ConnectionMediaType(&mtsrc);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}
	if (mtsrc.majortype != MEDIATYPE_Audio/* || mtsrc.subtype != MEDIASUBTYPE_PCM*/)
	{
		fprintf(stderr, "error : illegal mediatype\n");
		return 1;
	}
	if (nChannels == 0)
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
		PrintError(hr);
		return 1;
	}
    pFilter1->Release();

	// 映像出力を停止
	hr = pGraph->FindFilterByName(L"Video Renderer", &pFilter1);
	if (SUCCEEDED(hr))
	{
		IPin *pPinVidio_I;
		hr = GetPin(pFilter1, PINDIR_INPUT, &pPinVidio_I);
		if (FAILED(hr))
		{
			PrintError(hr);
			return 1;
		}
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
		PrintError(hr);
		return 1;
	}



	// RendererDstを生成
	CRendererDst *pRendererDst = new CRendererDst(outfile, &hr);
	if (hr != S_OK)
	{
		PrintError(hr);
		return 1;
	}

	pRendererDst->AddRef();

	hr = pGraph->AddFilter(pRendererDst, L"Renderer Dst");
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}

	// ACMWrapperのPIN取得
	IPin *pPinACMWrapper_In;
	hr = GetPin(pACMWrapper, PINDIR_INPUT, &pPinACMWrapper_In);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}

	IPin *pPinACMWrapper_Out;
	hr = GetPin(pACMWrapper, PINDIR_OUTPUT, &pPinACMWrapper_Out);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}

	// Render DstのPIN取得
	IPin *pPinRendererDst_In;
	hr = GetPin(pRendererDst, PINDIR_INPUT, &pPinRendererDst_In);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}

	// 接続
	hr = pGraph->Connect(pPinSrc_O, pPinACMWrapper_In);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}

	hr = pGraph->Connect(pPinACMWrapper_Out, pPinRendererDst_In);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}

	pPinSrc_O->Release();
	pPinACMWrapper_In->Release();
	pPinRendererDst_In->Release();

	PrintFilters(pGraph);

	// サンプリングレート設定(メモ：PIN接続後)
	if (argc > 3)
	{

		IAMStreamConfig *pAMStreamCfg = NULL;
		pPinACMWrapper_Out->QueryInterface(IID_IAMStreamConfig, (void **)&pAMStreamCfg);

		WAVEFORMATEX wf = {0};
		wf.nChannels       = nChannels;
		wf.nSamplesPerSec  = nSamplesPerSec;
		wf.wBitsPerSample  = wBitsPerSample;
		wf.nBlockAlign     = wf.nChannels * wf.wBitsPerSample / 8;
		wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
		wf.wFormatTag      = WAVE_FORMAT_PCM;

		AM_MEDIA_TYPE mt = {0};
		mt.majortype         = MEDIATYPE_Audio;
		mt.subtype           = MEDIASUBTYPE_PCM;
		mt.bFixedSizeSamples = TRUE;
		mt.lSampleSize       = wf.nBlockAlign;
		mt.formattype        = FORMAT_WaveFormatEx;
		mt.cbFormat          = sizeof(WAVEFORMATEX);
		mt.pbFormat          = (BYTE *)&wf;

		hr = pAMStreamCfg->SetFormat(&mt);
		if (FAILED(hr))
		{
			PrintError(hr);
			return 1;
		}
		pAMStreamCfg->Release();
	}
	pPinACMWrapper_Out->Release();

	//MediaControlインターフェース取得
	IMediaControl *pMediaControl;
	hr = pGraph->QueryInterface(IID_IMediaControl,(LPVOID *)&pMediaControl);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}

	IMediaFilter *pMediaFilter;
	pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
	hr = pMediaFilter->SetSyncSource(NULL);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}
	pMediaFilter->Release();

	// 再生
	hr = pMediaControl->Run();
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}

	// 終了待機
	IMediaEvent *pEvent;
	hr = pGraph->QueryInterface(IID_IMediaEvent,(LPVOID *)&pEvent);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 1;
	}

	long evCode;
	hr = pEvent->WaitForCompletion(INFINITE, &evCode);
	if (FAILED(hr))
	{
		PrintError(hr);
		return 0;
	}

	hr = pMediaControl->Stop();
	if (FAILED(hr))
	{
		PrintError(hr);
		return 0;
	}

    pMediaControl->Release();
	pEvent->Release();
	pRendererDst->Release();
	pACMWrapper->Release();
    pGraph->Release();

	CoUninitialize();

	return 0;
}

void PrintError_Imp(HRESULT hr, wchar_t *file, int line)
{
	wchar_t lpBuffer[MAX_ERROR_TEXT_LEN+1] = {0};
	DWORD ret = AMGetErrorText(hr, lpBuffer, MAX_ERROR_TEXT_LEN);
	wprintf(L"error : %x %s (%s,%d)\n", hr, lpBuffer, file, line);
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

		wprintf(L" < %s", FilterInfo.achName);
		pFilter->Release();
	}
	_putws(L"\n");
	pEnum->Release();
}

HRESULT GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
	IEnumPins  *pEnum = NULL;
	IPin       *pPin = NULL;
	HRESULT    hr;

	if (ppPin == NULL)
	{
		return E_POINTER;
	}

	hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	while(pEnum->Next(1, &pPin, 0) == S_OK)
	{
		PIN_DIRECTION PinDirThis;
		hr = pPin->QueryDirection(&PinDirThis);
		if (FAILED(hr))
		{
			pPin->Release();
			pEnum->Release();
			return hr;
		}
		if (PinDir == PinDirThis)
		{
			// Found a match. Return the IPin pointer to the caller.
			*ppPin = pPin;
			pEnum->Release();
			return S_OK;
		}
		// Release the pin for the next time through the loop.
		pPin->Release();
	}
	// No more pins. We did not find a match.
	pEnum->Release();
	return E_FAIL;  
}

void PrintUsage()
{
	printf("usage : wavext [Channels SamplesPerSec BitsPerSample] [<file>] [<output file>]\n"
		"    Channels        1:mono 2:stereo\n"
		"    SamplesPerSec   8000 11025 16000 22050 32000 44100 etc\n"
		"    BitsPerSample   8 16\n\n"
		"    example)   wavext test.mp3\n"
		"               wavext 2 44100 16 test.mp3\n"
		"               wavext test.mp3 out.wav\n"
		"               wavext 2 44100 16 test.mp3 out.wav\n"
		);
}