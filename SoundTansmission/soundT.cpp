#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <WS2tcpip.h>
#include <MMSystem.h>
#include <string.h>
#include <fstream>


#pragma comment( lib, "winmm" ) //--подключаем winmm.dll--

using namespace std;

#define IDC_B1 (1234) //--кнопка воспроизведения--
#define IDC_B2 (1235) //--кнопка выхода--
#define IDC_B3 (1237) //--кнопка начала передачи--
#define IDC_E1 (1236) //--текстовое поле для вывода ошибок--

LRESULT CALLBACK MyFunc(HWND, UINT, WPARAM, LPARAM);

char szWinName[] = "MyWin";
string filename = "C:\\MoonlightSonata.wav";

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
bool state[sizeWaveHdr]; //--показывает буфер полон или нет--
int bN = 0; //--номер следующего по списку свободного буфера--
int emptyBuffer = 0; //--номер очищаемого буфера--

//--Объявления функциий--------------------------------------------------------------------------------------

DWORD WINAPI ServerThread(LPVOID pParam);
DWORD WINAPI ClientThread(LPVOID pParam);
void PrintText(string text);			//--печать ошибок--
void PrintText(int number);				//--печать ошибок--
int ParseData();						//--создание аудиопотока--
void PlayBuffer(char *Buffer);			//--воспроизведение буфера--
DWORD WINAPI PlaySound(LPVOID pParam);  //--воспроизведение--
DWORD WINAPI PlaySoundTest(LPVOID pParam);

//--Сердце программы-----------------------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst, LPSTR lpszArgs, int nWinMode)
{
	MSG msg;
	WNDCLASS wcl;

	HWND hButton1, hButton2, hButton3;

	wcl.hInstance = NULL;// hThisInst;
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

	CreateWindowA(
		"BUTTON", "Start", WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
		100, 370, 100, 24, hWnd, (HMENU)IDC_B3, NULL, NULL);

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

			/*waveOutUnprepareHeader(hWaveOut, &waveHdr[emptyBuffer], sizeof(waveHdr));
			free(waveHdr[emptyBuffer].lpData);*/

			state[emptyBuffer] = false;

			emptyBuffer = (emptyBuffer == 7) ? 0 : emptyBuffer + 1;

			//--Действие если аудиопоток закончился----------------------------------------------------------

			PrintText("Конец воспроизведения");
		}
		break;

	case WM_COMMAND:
		{
			int idButton = (int) LOWORD(wParam); // идентификатор, который указан в CreateWindowEx

			if(idButton == IDC_B1)
			{
				HANDLE	hThread1;
				DWORD	dwThreadId1;

				if(ParseData()) return 0;
				strcpy(sound_buffer,"");
				size = 0;

				hThread1 = CreateThread(NULL, 0, ClientThread, NULL, 0, &dwThreadId1);


				HANDLE	hThread2;
				DWORD	dwThreadId2;

				hThread2 = CreateThread(NULL, 0, PlaySoundTest, NULL, 0, &dwThreadId2);
				//--Нужно добавить создание потока--
				//--Этот поток должен работать так же как и PlaySound()--
			}

			if(idButton == IDC_B2)
			{
				HANDLE	hThread1;
				DWORD	dwThreadId1;

				hThread1 = CreateThread(NULL, 0, PlaySound, NULL, 0, &dwThreadId1);

				//SendMessage(hWnd, WM_DESTROY, 0, 0);
			}

			if(idButton == IDC_B3)
			{
				HANDLE	hThread2;
				DWORD	dwThreadId2;
				hThread2 = CreateThread(NULL, 0, ServerThread, NULL, 0, &dwThreadId2);
				//--Нужно добавить создание потока--
				//--Поток должен каждый раз обращатся к функции PlayBuffer()--
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

	//state[bN] = true;

	////--Подготовка буфера для записи-------------------------------------------------------------------------

	//ZeroMemory(&waveHdr[bN], sizeof(waveHdr[bN])); //--обнуляем WaveHdr--

	//waveHdr[bN].lpData = Buffer; //--присваиваем аудиопоток для воспроизведения--
	//waveHdr[bN].dwBufferLength = WaveFormat.nSamplesPerSec; //--размер аудиопотока--
	//waveHdr[bN].dwFlags = WHDR_INQUEUE;
	//waveHdr[bN].dwLoops = 1;
	////--Заполнение аудиопотока-------------------------------------------------------------------------------

	//waveOutPrepareHeader(hWaveOut, &waveHdr[bN], sizeof(WAVEHDR));

	//waveOutWrite(hWaveOut, &waveHdr[bN], sizeof(WAVEHDR));

	//NextBufNumber();

	WAVEHDR* whdr = new WAVEHDR();
	ZeroMemory(&(*whdr), sizeof((*whdr))); //--обнуляем WaveHdr--

	(*whdr).lpData = Buffer; //--присваиваем аудиопоток для воспроизведения--
	(*whdr).dwBufferLength = WaveFormat.nSamplesPerSec; //--размер аудиопотока--
	(*whdr).dwFlags = WHDR_INQUEUE;
	(*whdr).dwLoops = 1;
	//--Заполнение аудиопотока-------------------------------------------------------------------------------

	waveOutPrepareHeader(hWaveOut, &(*whdr), sizeof(WAVEHDR));

	waveOutWrite(hWaveOut, &(*whdr), sizeof(WAVEHDR));

}

//--Воспроизводим аудиофайл----------------------------------------------------------------------------------

DWORD WINAPI PlaySound(LPVOID pParam)
{
	//--ВыДеление данных из WAV файла------------------------------------------------------------------------

	if(ParseData()) return 0;

	//--Открытие аудиоустройства-----------------------------------------------------------------------------

	MMRESULT mmRes = 
		waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW);

	if(mmRes != 0) PrintText("(waveOpen) Error code: " + mmRes); //--вывод ошибок если они есть--

	char *Buffer = new char[WaveFormat.nSamplesPerSec];

	for(int i=0; i < (size/WaveFormat.nSamplesPerSec); ++i)
	{
		Buffer = (sound_buffer + (i * WaveFormat.nSamplesPerSec));
		Sleep(100);
		PlayBuffer(Buffer);
	}
}

DWORD WINAPI PlaySoundTest(LPVOID pParam)
{
	//--ВыДеление данных из WAV файла------------------------------------------------------------------------

	//--Открытие аудиоустройства-----------------------------------------------------------------------------

	MMRESULT mmRes = 
		waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW);

	if(mmRes != 0) PrintText("(waveOpen) Error code: " + mmRes); //--вывод ошибок если они есть--

	char *Buffer = new char[WaveFormat.nSamplesPerSec];

	int i=0;

	while(1)
	{
		while(size < ((i+1) * WaveFormat.nSamplesPerSec)){}
		Buffer = (sound_buffer + (i * WaveFormat.nSamplesPerSec));
		Sleep(100);
		PlayBuffer(Buffer);
		i++;
	}
}

#pragma endregion

#pragma region WinSocket

//--Server---------------------------------------------------------------------------------------------------

DWORD WINAPI ServerThread(LPVOID pParam)
{
	// Инициализируем библиотеку Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
	if ( iResult != NO_ERROR )
		PrintText("Error at WSAStartup()\n");

	// Получаем данные для инициализации
	struct addrinfo *res = NULL, hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	if ( getaddrinfo("127.0.0.1", "27015", &hints, &res) != 0 )
	{// обработка ошибки 
		PrintText("Can't get initiaalization data!\n");
		WSACleanup();
		return 0;
	}
	// Инициализируем сокет
	SOCKET m_socket;
	m_socket = socket(res->ai_family, res->ai_socktype,res->ai_protocol);
	if ( m_socket == INVALID_SOCKET ) 
	{ // Обработка ошибки инициализации
		PrintText( "Error at socket(): %ld\n" );
		freeaddrinfo(res);
		WSACleanup();
		return 0;
	}
	if ( bind( m_socket, res->ai_addr,res->ai_addrlen) == SOCKET_ERROR ) 
	{ // Обработка ошибки связывания
		PrintText( "bind() failed.\n" );
		freeaddrinfo(res);
		closesocket(m_socket);
		WSACleanup();
		return 0;
	}

	// Переводим сокет с пассивное состояние - режим прослушивания.
	if ( listen( m_socket, 1 ) == SOCKET_ERROR )
		PrintText( "Error listening on socket.\n");

	// Принимаем входящее соединение.
	SOCKET AcceptSocket;

	PrintText( "Waiting for a client to connect...\n" );
	while (1) {
		AcceptSocket = SOCKET_ERROR;
		while ( AcceptSocket == SOCKET_ERROR ) {
			AcceptSocket = accept( m_socket, NULL, NULL );
		}
		PrintText("Client Connected.\n");
		closesocket(m_socket); 
		break;
	}
	m_socket = AcceptSocket; 

	// Получаем и передаем данные.
	int bytesSent;

	while(1)
	{
		//--ВыДеление данных из WAV файла------------------------------------------------------------------------

		if(ParseData()) return 0;

		//--Открытие аудиоустройства-----------------------------------------------------------------------------

		//MMRESULT mmRes = 
		//	waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW);

		//if(mmRes != 0) PrintText("(waveOpen) Error code: " + mmRes); //--вывод ошибок если они есть--

		char *Buffer = new char[WaveFormat.nSamplesPerSec];

		for(int i=0; i < (size/WaveFormat.nSamplesPerSec); ++i)
		{
			strcpy(Buffer,(sound_buffer + (i * WaveFormat.nSamplesPerSec)));
			Sleep(100);

			bytesSent = send( m_socket, Buffer, WaveFormat.nSamplesPerSec, 0 );
			PrintText(i); PrintText(" - transmitted");
		}
	}

	// Корректно завершаем работу
	// Отключаем сокет
	shutdown(m_socket,SD_BOTH);
	// Освобождаем ресурсы, занятые информацией об адресе
	freeaddrinfo(res);
	// Закрываем сокет
	closesocket(m_socket);
	// Отключаем библиотеку Winsock
	WSACleanup();

	return 0;
}
//--Client---------------------------------------------------------------------------------------------------

DWORD WINAPI ClientThread(LPVOID pParam){
	// Инициализируем библиотеку Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
	if ( iResult != NO_ERROR )
		PrintText("Error at WSAStartup()\n");

	// Получаем данные для инициализации
	struct addrinfo *res = NULL, hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	if ( getaddrinfo("127.0.0.1", "27015", &hints, &res) != 0 )
	{// обработка ошибки 
		PrintText("Can't get initiaalization data!\n");
		WSACleanup();
		return 0;
	}
	// Инициализируем сокет
	SOCKET m_socket;
	m_socket = socket(res->ai_family, res->ai_socktype,res->ai_protocol);
	if ( m_socket == INVALID_SOCKET ) 
	{ // Обработка ошибки инициализации
		PrintText( "Error at socket(): %ld\n");
		freeaddrinfo(res);
		WSACleanup();
		return 0;
	}
	// Подсоединяемся к серверу
	if ( connect( m_socket, res->ai_addr,res->ai_addrlen) == SOCKET_ERROR) {
		PrintText( "Failed to connect.\n" );
		closesocket(m_socket);
		freeaddrinfo(res);
		WSACleanup();
		return 0;
	}

	// Передаем и получаем данные.
	int bytesRecv = SOCKET_ERROR;

	/*bytesSent = send( m_socket, sendbuf, strlen(sendbuf), 0 );
	printf( "Bytes Sent: %ld\n", bytesSent );*/
	int k=0;

	//MMRESULT mmRes = 
	//	waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW);

	//if(mmRes != 0) PrintText("(waveOpen) Error code: " + mmRes); //--вывод ошибок если они есть--
	
	char *Buffer = new char[WaveFormat.nSamplesPerSec];

	while( 1 ) {
		bytesRecv = recv( m_socket, Buffer, WaveFormat.nSamplesPerSec, 0 );
		strcat(sound_buffer,Buffer);
		size += WaveFormat.nSamplesPerSec;
		PrintText(k++); PrintText(" - add");
	}


	// Корректно завершаем работу
	// Отключаем сокет
	shutdown(m_socket,SD_BOTH);
	// Освобождаем ресурсы, занятые информацией об адресе
	freeaddrinfo(res);
	// Закрываем сокет
	closesocket(m_socket);
	// Отключаем библиотеку Winsock
	WSACleanup();

	return 0;
}

#pragma endregion