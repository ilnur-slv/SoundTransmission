#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <windows.h>
#include <MMSystem.h>
#include <string.h>
#pragma comment( lib, "winmm" ) //--���������� winmm.dll--

using namespace std;

HWND hWnd;
HWAVEIN hWaveIn;
WAVEHDR WaveHdr; //--��������� ������--
WAVEFORMATEX WaveFormat; //--������ ������--

char* ParseData(string path = "C:\\spasti.wav")
{
	HMMIO hMmio = mmioOpen(strdup(path.c_str()), NULL, MMIO_READ | MMIO_ALLOCBUF); //--��������� RIFF ����. ��������� �����������--

	cout << "(mmioOpen) " << ( (hMmio != 0) ? "No errors" : "Error code: 0" ) << endl; //--����� ������ ���� ��� ����--

	//--(WAVE) ���� ���� ������ �������, ������ RIFF ���������-----------------------------------------------

	MMCKINFO mmCkInfoRiff;
	mmCkInfoRiff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	MMRESULT mmRes = mmioDescend(hMmio, &mmCkInfoRiff, NULL, MMIO_FINDRIFF);

	cout << "(RIFF WAVE) " << ( (mmRes == MMSYSERR_NOERROR) ? "No errors" : "Error" ) << endl; //--����� ������ ���� ��� ����--

	//--(FMT) ������ RIFF ���������--------------------------------------------------------------------------

	MMCKINFO mmCkInfo;
	mmCkInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
	mmRes = mmioDescend(hMmio, &mmCkInfo, &mmCkInfoRiff, MMIO_FINDCHUNK);

	cout << "(RIFF fmt) " << ( (mmRes == MMSYSERR_NOERROR) ? "No errors" : "Error" ) << endl; //--����� ������ ���� ��� ����--

	//--(DATA) ������ RIFF ���������. ������ ����������� � WaveFormat----------------------------------------



	//--��������� RIFF ����, ��� fuClose - ��� ��� ������----------------------------------------------------

	UINT fuClose;
	mmioClose(hMmio, fuClose); //--��������� RIFF ����--

	return "0";
}

void PlaySound()
{
	//--�������� ���������������-----------------------------------------------------------------------------

	WaveFormat.wFormatTag = WAVE_FORMAT_PCM; //--��� �����������--
	WaveFormat.nChannels = 1; //--���������� �������--
	WaveFormat.nSamplesPerSec = 44100; //--����� ������ � �������. ������ 44100 ��--
	WaveFormat.nBlockAlign = 2; //--������������ � ������--
	WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign; //--�������� �������� ������ � ���.--
	WaveFormat.wBitsPerSample = 16; //--���������� ��� ��� �������--
	WaveFormat.cbSize = 20; //--������ ���� ���������--

	MMRESULT mmRes = 
		waveInOpen(&hWaveIn, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW); //--��������� ���������������--

	cout << "(waveOpen) " << ((mmRes == 0) ? "No errors" : "error code: " + mmRes) << endl; //--����� ������ ���� ��� ����--

	//--���������� ������ ��� ������-------------------------------------------------------------------------

	WAVEHDR WaveHdr;
	ULONG BufferSize = WaveFormat.nBlockAlign*WaveFormat.nSamplesPerSec*10;
	WaveHdr.lpData = (char*)malloc(BufferSize); //--��������� �� ����� ��� �������� ���������� ������--
	WaveHdr.dwBufferLength = BufferSize; //--������ ������--

	//--���������� ������------------------------------------------------------------------------------------

	//--���������� ������ ���� ��� ��� ������� ���������������� � ������� �������� ������--
	waveInPrepareHeader(hWaveIn, &WaveHdr, sizeof(WAVEHDR));
	waveInAddBuffer(hWaveIn, &WaveHdr, sizeof(WAVEHDR)); //--������ � ������� �������� � �����--

	//--�������� ������--------------------------------------------------------------------------------------
	waveInStart(hWaveIn);

	waveInClose(hWaveIn); //--��������� ���������������--
}

int main()
{
	ParseData();
	return 0;
}