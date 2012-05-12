#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <WS2tcpip.h>
#include <MMSystem.h>
#include <string.h>
#include <fstream>


#pragma comment( lib, "winmm" ) //--���������� winmm.dll--

using namespace std;

#define IDC_B1 (1234) //--������ ���������������--
#define IDC_B2 (1235) //--������ ������--
#define IDC_B3 (1237) //--������ ������ ��������--
#define IDC_E1 (1236) //--��������� ���� ��� ������ ������--

LRESULT CALLBACK MyFunc(HWND, UINT, WPARAM, LPARAM);

char szWinName[] = "MyWin";
string filename = "C:\\MoonlightSonata.wav";

HWND hWnd; //--������� ����--
HWND hEdit; //--��������� ����--
HWAVEIN hWaveIn; //--��������� ������--
WAVEHDR WaveHdr; //--��������� ������--
WAVEFORMATEX WaveFormat; //--������ ������--
HWAVEOUT hWaveOut; //--���������� ������--
char *sound_buffer; //--����������--
int size; //--������ �����������--
const int sizeWaveHdr = 8; //--���������� ������������--
WAVEHDR waveHdr[sizeWaveHdr]; //--������ ��� ������������--
bool state[sizeWaveHdr]; //--���������� ����� ����� ��� ���--
int bN = 0; //--����� ���������� �� ������ ���������� ������--
int emptyBuffer = 0; //--����� ���������� ������--

//--���������� ��������--------------------------------------------------------------------------------------

DWORD WINAPI ServerThread(LPVOID pParam);
DWORD WINAPI ClientThread(LPVOID pParam);
void PrintText(string text);			//--������ ������--
void PrintText(int number);				//--������ ������--
int ParseData();						//--�������� �����������--
void PlayBuffer(char *Buffer);			//--��������������� ������--
DWORD WINAPI PlaySound(LPVOID pParam);  //--���������������--
DWORD WINAPI PlaySoundTest(LPVOID pParam);

//--������ ���������-----------------------------------------------------------------------------------------

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
			//--������� ������-------------------------------------------------------------------------------

			/*waveOutUnprepareHeader(hWaveOut, &waveHdr[emptyBuffer], sizeof(waveHdr));
			free(waveHdr[emptyBuffer].lpData);*/

			state[emptyBuffer] = false;

			emptyBuffer = (emptyBuffer == 7) ? 0 : emptyBuffer + 1;

			//--�������� ���� ���������� ����������----------------------------------------------------------

			PrintText("����� ���������������");
		}
		break;

	case WM_COMMAND:
		{
			int idButton = (int) LOWORD(wParam); // �������������, ������� ������ � CreateWindowEx

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
				//--����� �������� �������� ������--
				//--���� ����� ������ �������� ��� �� ��� � PlaySound()--
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
				//--����� �������� �������� ������--
				//--����� ������ ������ ��� ��������� � ������� PlayBuffer()--
			}

			break;
		}

		break;

	}

	return DefWindowProc(this_hwnd, message, wParam, lParam);
}

#pragma endregion

#pragma region Standart funtion

//--������ ������ � ������-----------------------------------------------------------------------------------

void PrintText(string text)
{
	text += "\n";
	int ndx = GetWindowTextLength (hEdit);
	SetFocus (hEdit);
	SendMessage (hEdit, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx); //--������������� ������--
	SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM)text.c_str()); //--��������� �� ����� ������� �����--
}

//--������ ����� � ������------------------------------------------------------------------------------------

void PrintText(int number)
{
	char buf[100];
	itoa(number,buf,10);

	string text = buf;

	int ndx = GetWindowTextLength (hEdit);
	SetFocus (hEdit);
	SendMessage (hEdit, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx); //--������������� ������--
	SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM)text.c_str()); //--��������� �� ����� ������� �����--
}

//--���������� ���������� ������-----------------------------------------------------------------------------

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

//--������ wav ���� � ���������� ������ ������---------------------------------------------------------------

int ParseData()
{
	//--��������� RIFF ����. ��������� �����������--
	HMMIO hMmio = mmioOpen(strdup(filename.c_str()), NULL, MMIO_READ | MMIO_ALLOCBUF);

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

//--������������� �����--------------------------------------------------------------------------------------

void PlayBuffer(char *Buffer){

	//ofstream file;
	//file.open("C:\\out.txt", ios_base::app);
	//file.binary;
	//for(int i=0; i < WaveFormat.nSamplesPerSec; ++i)
	//file << *(Buffer + i);
	//file.close();

	//state[bN] = true;

	////--���������� ������ ��� ������-------------------------------------------------------------------------

	//ZeroMemory(&waveHdr[bN], sizeof(waveHdr[bN])); //--�������� WaveHdr--

	//waveHdr[bN].lpData = Buffer; //--����������� ���������� ��� ���������������--
	//waveHdr[bN].dwBufferLength = WaveFormat.nSamplesPerSec; //--������ �����������--
	//waveHdr[bN].dwFlags = WHDR_INQUEUE;
	//waveHdr[bN].dwLoops = 1;
	////--���������� �����������-------------------------------------------------------------------------------

	//waveOutPrepareHeader(hWaveOut, &waveHdr[bN], sizeof(WAVEHDR));

	//waveOutWrite(hWaveOut, &waveHdr[bN], sizeof(WAVEHDR));

	//NextBufNumber();

	WAVEHDR* whdr = new WAVEHDR();
	ZeroMemory(&(*whdr), sizeof((*whdr))); //--�������� WaveHdr--

	(*whdr).lpData = Buffer; //--����������� ���������� ��� ���������������--
	(*whdr).dwBufferLength = WaveFormat.nSamplesPerSec; //--������ �����������--
	(*whdr).dwFlags = WHDR_INQUEUE;
	(*whdr).dwLoops = 1;
	//--���������� �����������-------------------------------------------------------------------------------

	waveOutPrepareHeader(hWaveOut, &(*whdr), sizeof(WAVEHDR));

	waveOutWrite(hWaveOut, &(*whdr), sizeof(WAVEHDR));

}

//--������������� ���������----------------------------------------------------------------------------------

DWORD WINAPI PlaySound(LPVOID pParam)
{
	//--��������� ������ �� WAV �����------------------------------------------------------------------------

	if(ParseData()) return 0;

	//--�������� ���������������-----------------------------------------------------------------------------

	MMRESULT mmRes = 
		waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW);

	if(mmRes != 0) PrintText("(waveOpen) Error code: " + mmRes); //--����� ������ ���� ��� ����--

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
	//--��������� ������ �� WAV �����------------------------------------------------------------------------

	//--�������� ���������������-----------------------------------------------------------------------------

	MMRESULT mmRes = 
		waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW);

	if(mmRes != 0) PrintText("(waveOpen) Error code: " + mmRes); //--����� ������ ���� ��� ����--

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
	// �������������� ���������� Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
	if ( iResult != NO_ERROR )
		PrintText("Error at WSAStartup()\n");

	// �������� ������ ��� �������������
	struct addrinfo *res = NULL, hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	if ( getaddrinfo("127.0.0.1", "27015", &hints, &res) != 0 )
	{// ��������� ������ 
		PrintText("Can't get initiaalization data!\n");
		WSACleanup();
		return 0;
	}
	// �������������� �����
	SOCKET m_socket;
	m_socket = socket(res->ai_family, res->ai_socktype,res->ai_protocol);
	if ( m_socket == INVALID_SOCKET ) 
	{ // ��������� ������ �������������
		PrintText( "Error at socket(): %ld\n" );
		freeaddrinfo(res);
		WSACleanup();
		return 0;
	}
	if ( bind( m_socket, res->ai_addr,res->ai_addrlen) == SOCKET_ERROR ) 
	{ // ��������� ������ ����������
		PrintText( "bind() failed.\n" );
		freeaddrinfo(res);
		closesocket(m_socket);
		WSACleanup();
		return 0;
	}

	// ��������� ����� � ��������� ��������� - ����� �������������.
	if ( listen( m_socket, 1 ) == SOCKET_ERROR )
		PrintText( "Error listening on socket.\n");

	// ��������� �������� ����������.
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

	// �������� � �������� ������.
	int bytesSent;

	while(1)
	{
		//--��������� ������ �� WAV �����------------------------------------------------------------------------

		if(ParseData()) return 0;

		//--�������� ���������������-----------------------------------------------------------------------------

		//MMRESULT mmRes = 
		//	waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW);

		//if(mmRes != 0) PrintText("(waveOpen) Error code: " + mmRes); //--����� ������ ���� ��� ����--

		char *Buffer = new char[WaveFormat.nSamplesPerSec];

		for(int i=0; i < (size/WaveFormat.nSamplesPerSec); ++i)
		{
			strcpy(Buffer,(sound_buffer + (i * WaveFormat.nSamplesPerSec)));
			Sleep(100);

			bytesSent = send( m_socket, Buffer, WaveFormat.nSamplesPerSec, 0 );
			PrintText(i); PrintText(" - transmitted");
		}
	}

	// ��������� ��������� ������
	// ��������� �����
	shutdown(m_socket,SD_BOTH);
	// ����������� �������, ������� ����������� �� ������
	freeaddrinfo(res);
	// ��������� �����
	closesocket(m_socket);
	// ��������� ���������� Winsock
	WSACleanup();

	return 0;
}
//--Client---------------------------------------------------------------------------------------------------

DWORD WINAPI ClientThread(LPVOID pParam){
	// �������������� ���������� Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
	if ( iResult != NO_ERROR )
		PrintText("Error at WSAStartup()\n");

	// �������� ������ ��� �������������
	struct addrinfo *res = NULL, hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	if ( getaddrinfo("127.0.0.1", "27015", &hints, &res) != 0 )
	{// ��������� ������ 
		PrintText("Can't get initiaalization data!\n");
		WSACleanup();
		return 0;
	}
	// �������������� �����
	SOCKET m_socket;
	m_socket = socket(res->ai_family, res->ai_socktype,res->ai_protocol);
	if ( m_socket == INVALID_SOCKET ) 
	{ // ��������� ������ �������������
		PrintText( "Error at socket(): %ld\n");
		freeaddrinfo(res);
		WSACleanup();
		return 0;
	}
	// �������������� � �������
	if ( connect( m_socket, res->ai_addr,res->ai_addrlen) == SOCKET_ERROR) {
		PrintText( "Failed to connect.\n" );
		closesocket(m_socket);
		freeaddrinfo(res);
		WSACleanup();
		return 0;
	}

	// �������� � �������� ������.
	int bytesRecv = SOCKET_ERROR;

	/*bytesSent = send( m_socket, sendbuf, strlen(sendbuf), 0 );
	printf( "Bytes Sent: %ld\n", bytesSent );*/
	int k=0;

	//MMRESULT mmRes = 
	//	waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)hWnd, 0L, CALLBACK_WINDOW);

	//if(mmRes != 0) PrintText("(waveOpen) Error code: " + mmRes); //--����� ������ ���� ��� ����--
	
	char *Buffer = new char[WaveFormat.nSamplesPerSec];

	while( 1 ) {
		bytesRecv = recv( m_socket, Buffer, WaveFormat.nSamplesPerSec, 0 );
		strcat(sound_buffer,Buffer);
		size += WaveFormat.nSamplesPerSec;
		PrintText(k++); PrintText(" - add");
	}


	// ��������� ��������� ������
	// ��������� �����
	shutdown(m_socket,SD_BOTH);
	// ����������� �������, ������� ����������� �� ������
	freeaddrinfo(res);
	// ��������� �����
	closesocket(m_socket);
	// ��������� ���������� Winsock
	WSACleanup();

	return 0;
}

#pragma endregion