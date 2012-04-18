#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <windows.h>
#include <MMSystem.h>
#include <string.h>
#include <fstream>
#pragma comment( lib, "winmm" ) //--���������� winmm.dll--

using namespace std;

#define IDC_B1 (1234) //--������ ���������������--
#define IDC_B2 (1235) //--������ ������--
#define IDC_E1 (1236) //--��������� ���� ��� ������ ������--

LRESULT CALLBACK MyFunc(HWND, UINT, WPARAM, LPARAM);

char szWinName[] = "MyWin";

HWND hWnd; //--������� ����--
HWND hEdit; //--��������� ����--
HWAVEIN hWaveIn; //--��������� ������--
WAVEHDR WaveHdr; //--��������� ������--
WAVEFORMATEX WaveFormat; //--������ ������--
HWAVEOUT hWaveOut; //--���������� ������--
char *sound_buffer; //--����������--
int size; //--������ �����������--

//--������ ������ � ������-----------------------------------------------------------------------------------

void PrintText(string text)
{
	text += "\n";
	int ndx = GetWindowTextLength (hEdit);
	SetFocus (hEdit);
	SendMessage (hEdit, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx); //--������������� ������--
	SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM)text.c_str()); //--��������� �� ����� ������� �����--
}

//--������ wav ���� � ���������� ������ ������---------------------------------------------------------------

int ParseData(string path)
{
	HMMIO hMmio = mmioOpen(strdup(path.c_str()), NULL, MMIO_READ | MMIO_ALLOCBUF); //--��������� RIFF ����. ��������� �����������--

	if(hMmio == 0) PrintText("(mmioOpen) Error code: 0"); //--����� ������ ���� ��� ����--

	//--(WAVE) ���� ���� ������ �������, ������ RIFF ���������-----------------------------------------------

	MMCKINFO mmCkInfoRiff;
	mmCkInfoRiff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	MMRESULT mmRes = mmioDescend(hMmio, &mmCkInfoRiff, NULL, MMIO_FINDRIFF);

	if(mmRes != MMSYSERR_NOERROR) PrintText("(RIFF WAVE) Error"); //--����� ������ ���� ��� ����--

	//--(FMT) ������ RIFF ���������--------------------------------------------------------------------------

	MMCKINFO mmCkInfo;
	mmCkInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
	mmRes = mmioDescend(hMmio, &mmCkInfo, &mmCkInfoRiff, MMIO_FINDCHUNK);

	if(mmRes != MMSYSERR_NOERROR) PrintText("(RIFF fmt) Error"); //--����� ������ ���� ��� ����--

	//--(WaveFormat) ������ RIFF ���������. ������ ���������� ����������� � WaveFormat-----------------------

	mmioRead(hMmio, (char*)&WaveFormat, sizeof(WaveFormat) );

	//--(DATA) ������ RIFF ���������-------------------------------------------------------------------------

	mmRes = mmioAscend(hMmio, &mmCkInfo, 0);
	mmCkInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
	mmRes = mmioDescend(hMmio, &mmCkInfo, &mmCkInfoRiff, MMIO_FINDCHUNK);

	if(mmRes != MMSYSERR_NOERROR)
	{
		PrintText("(RIFF data) Error"); //--����� ������ ���� ��� ����--
		return 1;
	}

	//--���!!! �� ����� �� �����. ��������� ����� �����------------------------------------------------------

	sound_buffer = new char[(int)VirtualAlloc(NULL, mmCkInfo.cksize, MEM_COMMIT, PAGE_READWRITE )];
	mmioRead(hMmio, (HPSTR)mmioRead(hMmio, (HPSTR)sound_buffer, mmCkInfo.cksize), mmCkInfo.cksize);

	size = mmCkInfo.cksize; //--������������� ������ ����������--

	//--��������� RIFF ����----------------------------------------------------------------------------------

	mmioClose(hMmio, 0); //--��������� RIFF ����--

	return 0;
}

//--������������� ���������----------------------------------------------------------------------------------

void PlaySound(string path = "C:\\spasti.wav")
{
	//--����������� �������� �����������---------------------------------------------------------------------

	//WaveFormat.wFormatTag = WAVE_FORMAT_PCM; //--��� �����������--
	//WaveFormat.nChannels = 1; //--���������� �������--
	//WaveFormat.nSamplesPerSec = 44100; //--����� ������ � �������. ������ 44100 ��--
	//WaveFormat.nBlockAlign = 2; //--������������ � ������--
	//WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign; //--�������� �������� ������ � ���.--
	//WaveFormat.wBitsPerSample = 16; //--���������� ��� ��� �������--
	//WaveFormat.cbSize = 20; //--������ ���� ���������--

	//--��������� ������ �� WAV �����------------------------------------------------------------------------

	if(ParseData(path)) return;

	//--�������� ���������������-----------------------------------------------------------------------------

	MMRESULT mmRes = 
		waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW); //--��������� ���������������--

	if(mmRes != 0) PrintText("(waveOpen) Error code: " + mmRes); //--����� ������ ���� ��� ����--

	//--���������� ������ ��� ������-------------------------------------------------------------------------

	ZeroMemory(&WaveHdr, sizeof(WaveHdr)); //--�������� WaveHdr--

	WaveHdr.lpData = sound_buffer; //--����������� ���������� ��� ���������������--
	WaveHdr.dwBufferLength = size; //--������ �����������--

	//--���������� �����������------------------------------------------------------------------------------------

	waveOutPrepareHeader(hWaveOut, &WaveHdr, sizeof(WAVEHDR));
	
	waveOutWrite(hWaveOut, &WaveHdr, sizeof(WAVEHDR));
}

//--������ ���������-----------------------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst, LPSTR lpszArgs, int nWinMode)
{
	MSG msg;
	WNDCLASS wcl;

	HWND hButton1, hButton2;

	wcl.hInstance = NULL; // hThisInst;
	wcl.lpszClassName = szWinName;
	wcl.lpfnWndProc = MyFunc;
	wcl.style = 0;
	wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcl.lpszMenuName = NULL;

	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;

	wcl.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);

	if(!RegisterClass(&wcl)) return 0;

	hWnd = CreateWindowA(szWinName, "Simple Window", 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500 /*CW_USEDEFAULT*/, 500 /*CW_USEDEFAULT*/, HWND_DESKTOP, NULL, 
		NULL/*hThisInst*/, NULL);

	CreateWindowA(
		"BUTTON", "Play", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
		100, 400, 100, 24, hWnd, (HMENU)IDC_B1, NULL, NULL);

	CreateWindowA(
		"BUTTON", "Exit", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
		300, 400, 100, 24, hWnd, (HMENU)IDC_B2, NULL, NULL);

	hEdit = CreateWindowA(
		"EDIT","",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE,
		0,0,300,300,hWnd, (HMENU)IDC_E1, NULL,NULL);

	ShowWindow(hWnd, SW_SHOW /*nWinMode*/);
	UpdateWindow(hWnd);

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

//--������������ �������-------------------------------------------------------------------------------------

LRESULT CALLBACK MyFunc(HWND this_hwnd,UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) 
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_PAINT:
		break;

	case MM_WOM_DONE:
		{
			//--������� ������-------------------------------------------------------------------------------

			waveOutUnprepareHeader(hWaveOut, &WaveHdr, sizeof(WAVEHDR));
			free(WaveHdr.lpData);
			waveInClose(hWaveIn);

			//--�������� ���� ���������� ����������----------------------------------------------------------

			PrintText("����� ���������������");
		}
		break;

	case WM_COMMAND:
		{
			int idButton = (int) LOWORD(wParam); // �������������, ������� ������ � CreateWindowEx

			if(idButton == IDC_B1)
			{
				PlaySound();
			}

			if(idButton == IDC_B2)
			{
				SendMessage(hWnd, WM_DESTROY, 0, 0);
			}

			break;
		}

		break;

	}

	return DefWindowProc(this_hwnd, message, wParam, lParam);
}
