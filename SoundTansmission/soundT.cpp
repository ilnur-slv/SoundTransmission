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
string filename = "C:\\spasti.wav";

HWND hWnd; //--главное окно--
HWND hEdit; //--текстовое поле--
HWAVEIN hWaveIn; //--устрйство записи--
WAVEHDR WaveHdr; //--заголовок буфера--
WAVEFORMATEX WaveFormat; //--формат записи--
HWAVEOUT hWaveOut; //--устройство вывода--
char *sound_buffer; //--аудиопоток--
int size; //--размер аудиопотока--
const int sizeWaveHdr = 8; //--количество аудиопотоков--
WAVEHDR waveHdr[sizeWaveHdr]; //--массив для аудиопотоков--
int bN = 0;

//--Объявления функциий--------------------------------------------------------------------------------------

void PrintText(string text);   //--печать ошибок--
int ParseData();               //--создание аудиопотока--
void PlayBuffer(char *Buffer); //--воспроизведение буфера--
void PlaySound();              //--воспроизведение--

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

#pragma region CALLBACK

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

			for(int i=0; i<sizeWaveHdr; ++i)
			{
				if(waveHdr[i].dwLoops == 1)
				{
					waveOutUnprepareHeader(hWaveOut, &waveHdr[i], sizeof(waveHdr));
					free(waveHdr[i].lpData);
				}
			}
			//waveOutClose(hWaveOut);

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

#pragma endregion

#pragma region Standart funtion

//--Печать текста в окошко-----------------------------------------------------------------------------------

void PrintText(string text)
{
	text += "\n";
	int ndx = GetWindowTextLength (hEdit);
	SetFocus (hEdit);
	SendMessage (hEdit, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx); //--Устанавливаем курсор--
	SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM)text.c_str()); //--Вставляем на место курсора текст--
}

//--Печать числа в окошко------------------------------------------------------------------------------------

void PrintText(int number)
{
	char buf[100];
	itoa(number,buf,10);

	string text = buf;

	int ndx = GetWindowTextLength (hEdit);
	SetFocus (hEdit);
	SendMessage (hEdit, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx); //--Устанавливаем курсор--
	SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM)text.c_str()); //--Вставляем на место курсора текст--
}

//--Нахождение свободного потока-----------------------------------------------------------------------------

int NextBufNumber()
{
	if(bN == sizeWaveHdr - 1)
		bN = 0;
	else
		bN += 1;
	return bN;
}

#pragma endregion

#pragma region MMaudio

//--Парсим wav файл и возвращаем буффер данных---------------------------------------------------------------

int ParseData()
{
	//--открываем RIFF файл. Разрешаем буферизацию--
	HMMIO hMmio = mmioOpen(strdup(filename.c_str()), NULL, MMIO_READ | MMIO_ALLOCBUF);

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

//--Воспроизводим буфер--------------------------------------------------------------------------------------

void PlayBuffer(char *Buffer){

	//ofstream file;
	//file.open("C:\\out.txt", ios_base::app);
	//file.binary;
	//for(int i=0; i < WaveFormat.nSamplesPerSec; ++i)
	//file << *(Buffer + i);
	//file.close();

	//--Подготовка буфера для записи-------------------------------------------------------------------------

	ZeroMemory(&waveHdr[bN], sizeof(waveHdr[bN])); //--обнуляем WaveHdr--

	waveHdr[bN].lpData = Buffer; //--присваиваем аудиопоток для воспроизведения--
	waveHdr[bN].dwBufferLength = WaveFormat.nSamplesPerSec; //--размер аудиопотока--
	waveHdr[bN].dwFlags = WHDR_INQUEUE;
	waveHdr[bN].dwLoops = 1;
	//--Заполнение аудиопотока-------------------------------------------------------------------------------

	waveOutPrepareHeader(hWaveOut, &waveHdr[bN], sizeof(waveHdr));
	waveOutWrite(hWaveOut, &waveHdr[bN], sizeof(waveHdr));
	NextBufNumber();
}

//--Воспроизводим аудиофайл----------------------------------------------------------------------------------

void PlaySound()
{
	//--ВыДеление данных из WAV файла------------------------------------------------------------------------

	if(ParseData()) return;

	//--Открытие аудиоустройства-----------------------------------------------------------------------------

	MMRESULT mmRes = 
		waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW);

	if(mmRes != 0) PrintText("(waveOpen) Error code: " + mmRes); //--вывод ошибок если они есть--

	char *Buffer = new char[WaveFormat.nSamplesPerSec];

	for(int i=0; i < (size/WaveFormat.nSamplesPerSec); ++i)
	{
		Buffer = (sound_buffer + (i * WaveFormat.nSamplesPerSec));
		PlayBuffer(Buffer);
	}
}

#pragma endregion

#pragma region WinSocket



#pragma endregion