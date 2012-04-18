#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <windows.h>
#include <MMSystem.h>
#include <string.h>
#include <fstream>
#pragma comment( lib, "winmm" ) //--подключаем winmm.dll--

using namespace std;

#define IDC_B1 (1234) //--кнопка воспроизведения--
#define IDC_B2 (1235) //--кнопка выхода--
#define IDC_E1 (1236) //--текстовое поле для вывода ошибок--

LRESULT CALLBACK MyFunc(HWND, UINT, WPARAM, LPARAM);

char szWinName[] = "MyWin";

HWND hWnd; //--главное окно--
HWND hEdit; //--текстовое поле--
HWAVEIN hWaveIn; //--устрйство записи--
WAVEHDR WaveHdr; //--заголовок буфера--
WAVEFORMATEX WaveFormat; //--формат записи--
HWAVEOUT hWaveOut; //--устройство вывода--
char *sound_buffer; //--аудиопоток--
int size; //--размер аудиопотока--

//--Печать текста в окошко-----------------------------------------------------------------------------------

void PrintText(string text)
{
	text += "\n";
	int ndx = GetWindowTextLength (hEdit);
	SetFocus (hEdit);
	SendMessage (hEdit, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx); //--Устанавливаем курсор--
	SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM)text.c_str()); //--Вставляем на место курсора текст--
}

//--Парсим wav файл и возвращаем буффер данных---------------------------------------------------------------

int ParseData(string path)
{
	HMMIO hMmio = mmioOpen(strdup(path.c_str()), NULL, MMIO_READ | MMIO_ALLOCBUF); //--открываем RIFF файл. Разрешаем буферизацию--

	if(hMmio == 0) PrintText("(mmioOpen) Error code: 0"); //--вывод ошибок если они есть--

	//--(WAVE) Если файл открыт успешно, читаем RIFF заголовок-----------------------------------------------

	MMCKINFO mmCkInfoRiff;
	mmCkInfoRiff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	MMRESULT mmRes = mmioDescend(hMmio, &mmCkInfoRiff, NULL, MMIO_FINDRIFF);

	if(mmRes != MMSYSERR_NOERROR) PrintText("(RIFF WAVE) Error"); //--вывод ошибок если они есть--

	//--(FMT) Читаем RIFF заголовок--------------------------------------------------------------------------

	MMCKINFO mmCkInfo;
	mmCkInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
	mmRes = mmioDescend(hMmio, &mmCkInfo, &mmCkInfoRiff, MMIO_FINDCHUNK);

	if(mmRes != MMSYSERR_NOERROR) PrintText("(RIFF fmt) Error"); //--вывод ошибок если они есть--

	//--(WaveFormat) Читаем RIFF заголовок. Формат аудиофайла записвываем в WaveFormat-----------------------

	mmioRead(hMmio, (char*)&WaveFormat, sizeof(WaveFormat) );

	//--(DATA) Читаем RIFF заголовок-------------------------------------------------------------------------

	mmRes = mmioAscend(hMmio, &mmCkInfo, 0);
	mmCkInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
	mmRes = mmioDescend(hMmio, &mmCkInfo, &mmCkInfoRiff, MMIO_FINDCHUNK);

	if(mmRes != MMSYSERR_NOERROR)
	{
		PrintText("(RIFF data) Error"); //--вывод ошибок если они есть--
		return 1;
	}

	//--Ура!!! Мы дошли до этого. Считываем аудио поток------------------------------------------------------

	sound_buffer = new char[(int)VirtualAlloc(NULL, mmCkInfo.cksize, MEM_COMMIT, PAGE_READWRITE )];
	mmioRead(hMmio, (HPSTR)mmioRead(hMmio, (HPSTR)sound_buffer, mmCkInfo.cksize), mmCkInfo.cksize);

	size = mmCkInfo.cksize; //--устанавливаем размер аудиофайла--

	//--Закрываем RIFF файл----------------------------------------------------------------------------------

	mmioClose(hMmio, 0); //--закрываем RIFF файл--

	return 0;
}

//--Воспроизводим аудиофайл----------------------------------------------------------------------------------

void PlaySound(string path = "C:\\spasti.wav")
{
	//--Настраиваем качество аудиопотока---------------------------------------------------------------------

	//WaveFormat.wFormatTag = WAVE_FORMAT_PCM; //--тип аудиоданных--
	//WaveFormat.nChannels = 1; //--количество каналов--
	//WaveFormat.nSamplesPerSec = 44100; //--норма отбора в секунду. Обычно 44100 Гц--
	//WaveFormat.nBlockAlign = 2; //--выравнивание в байтах--
	//WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign; //--скорость передачи байтов в сек.--
	//WaveFormat.wBitsPerSample = 16; //--количество бит для выборки--
	//WaveFormat.cbSize = 20; //--размер всей структуры--

	//--ВыДеление данных из WAV файла------------------------------------------------------------------------

	if(ParseData(path)) return;

	//--Открытие аудиоустройства-----------------------------------------------------------------------------

	MMRESULT mmRes = 
		waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW); //--открываем аудиоустройство--

	if(mmRes != 0) PrintText("(waveOpen) Error code: " + mmRes); //--вывод ошибок если они есть--

	//--Подготовка буфера для записи-------------------------------------------------------------------------

	ZeroMemory(&WaveHdr, sizeof(WaveHdr)); //--обнуляем WaveHdr--

	WaveHdr.lpData = sound_buffer; //--присваиваем аудиопоток для воспроизведения--
	WaveHdr.dwBufferLength = size; //--размер аудиопотока--

	//--Заполнение аудиопотока------------------------------------------------------------------------------------

	waveOutPrepareHeader(hWaveOut, &WaveHdr, sizeof(WAVEHDR));
	
	waveOutWrite(hWaveOut, &WaveHdr, sizeof(WAVEHDR));
}

//--Сердце программы-----------------------------------------------------------------------------------------

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

//--Обрабатываем события-------------------------------------------------------------------------------------

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
			//--Очищаем память-------------------------------------------------------------------------------

			waveOutUnprepareHeader(hWaveOut, &WaveHdr, sizeof(WAVEHDR));
			free(WaveHdr.lpData);
			waveInClose(hWaveIn);

			//--Действие если аудиопоток закончился----------------------------------------------------------

			PrintText("Конец воспроизведения");
		}
		break;

	case WM_COMMAND:
		{
			int idButton = (int) LOWORD(wParam); // идентификатор, который указан в CreateWindowEx

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
