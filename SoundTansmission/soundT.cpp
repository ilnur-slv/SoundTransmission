#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <windows.h>
#include <MMSystem.h>
#include <string.h>
#pragma comment( lib, "winmm" ) //--подключаем winmm.dll--

using namespace std;

HWND hWnd;
HWAVEIN hWaveIn;
WAVEHDR WaveHdr; //--заголовок буфера--
WAVEFORMATEX WaveFormat; //--формат записи--

char* ParseData(string path = "C:\\spasti.wav")
{
	HMMIO hMmio = mmioOpen(strdup(path.c_str()), NULL, MMIO_READ | MMIO_ALLOCBUF); //--открываем RIFF файл. Разрешаем буферизацию--

	cout << "(mmioOpen) " << ( (hMmio != 0) ? "No errors" : "Error code: 0" ) << endl; //--вывод ошибок если они есть--

	//--(WAVE) Если файл открыт успешно, читаем RIFF заголовок-----------------------------------------------

	MMCKINFO mmCkInfoRiff;
	mmCkInfoRiff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	MMRESULT mmRes = mmioDescend(hMmio, &mmCkInfoRiff, NULL, MMIO_FINDRIFF);

	cout << "(RIFF WAVE) " << ( (mmRes == MMSYSERR_NOERROR) ? "No errors" : "Error" ) << endl; //--вывод ошибок если они есть--

	//--(FMT) Читаем RIFF заголовок--------------------------------------------------------------------------

	MMCKINFO mmCkInfo;
	mmCkInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
	mmRes = mmioDescend(hMmio, &mmCkInfo, &mmCkInfoRiff, MMIO_FINDCHUNK);

	cout << "(RIFF fmt) " << ( (mmRes == MMSYSERR_NOERROR) ? "No errors" : "Error" ) << endl; //--вывод ошибок если они есть--

	//--(DATA) Читаем RIFF заголовок. Данные записвываем в WaveFormat----------------------------------------



	//--Закрываем RIFF файл, где fuClose - это код ошибки----------------------------------------------------

	UINT fuClose;
	mmioClose(hMmio, fuClose); //--закрываем RIFF файл--

	return "0";
}

void PlaySound()
{
	//--Открытие аудиоустройства-----------------------------------------------------------------------------

	WaveFormat.wFormatTag = WAVE_FORMAT_PCM; //--тип аудиоданных--
	WaveFormat.nChannels = 1; //--количество каналов--
	WaveFormat.nSamplesPerSec = 44100; //--норма отбора в секунду. Обычно 44100 Гц--
	WaveFormat.nBlockAlign = 2; //--выравнивание в байтах--
	WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign; //--скорость передачи байтов в сек.--
	WaveFormat.wBitsPerSample = 16; //--количество бит для выборки--
	WaveFormat.cbSize = 20; //--размер всей структуры--

	MMRESULT mmRes = 
		waveInOpen(&hWaveIn, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW); //--открываем аудиоустройство--

	cout << "(waveOpen) " << ((mmRes == 0) ? "No errors" : "error code: " + mmRes) << endl; //--вывод ошибок если они есть--

	//--Подготовка буфера для записи-------------------------------------------------------------------------

	WAVEHDR WaveHdr;
	ULONG BufferSize = WaveFormat.nBlockAlign*WaveFormat.nSamplesPerSec*10;
	WaveHdr.lpData = (char*)malloc(BufferSize); //--указатель на буфер где хранятся записанные данные--
	WaveHdr.dwBufferLength = BufferSize; //--размер буфера--

	//--Заполнение буфера------------------------------------------------------------------------------------

	//--вызывается только один раз для каждого устанавливаемого в очередь загрузки буфера--
	waveInPrepareHeader(hWaveIn, &WaveHdr, sizeof(WAVEHDR));
	waveInAddBuffer(hWaveIn, &WaveHdr, sizeof(WAVEHDR)); //--ставит в очередь загрузки в буфер--

	//--Начинаем запись--------------------------------------------------------------------------------------
	waveInStart(hWaveIn);

	waveInClose(hWaveIn); //--закрываем аудиоустройство--
}

int main()
{
	ParseData();
	return 0;
}