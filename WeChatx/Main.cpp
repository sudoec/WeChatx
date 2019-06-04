#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <mutex>
#include <string>
#include <winsock2.h>
#include <Windows.h>
#include <Tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <shlobj.h>
#include <atlimage.h>
#include <opencv2/opencv.hpp>
#include "asio/asio.hpp"
#include "Timer.hpp"

using namespace cv;
using namespace std;
using asio::ip::tcp;

#define OFFSET 46

HWND hq;
RECT windowsize;
Mat preImg, curImg;
static bool outDate = true;
static bool commandFinish = true;
static std::string commandStr;
static int mleft, mtop;


void SendNum(char c)
{
	int code;
	if (c == '.')
	{
		code = 0xBE;
	}
	else if (c == '-')
	{
		code = 0x08;
	}
	else if (c <= '9' && c >= '0')
	{
		code = c;
	}
	else
	{
		return;
	}
	INPUT input[2];
	memset(input, 0, sizeof(input));
	input[0].type = input[1].type = INPUT_KEYBOARD;
	input[0].ki.wVk = input[1].ki.wVk = code;
	input[1].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(2, input, sizeof(INPUT));
}

void SendCtlC()
{
	INPUT input[4];
	memset(input, 0, sizeof(input));
	input[0].type = input[1].type = input[2].type = input[3].type = INPUT_KEYBOARD;
	input[0].ki.wVk = input[3].ki.wVk = VK_CONTROL;
	input[1].ki.wVk = input[2].ki.wVk = 'C';
	input[2].ki.dwFlags = input[3].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(4, input, sizeof(INPUT));
	Sleep(20);
}

void SendCtlV()
{
	INPUT input[4];
	memset(input, 0, sizeof(input));
	input[0].type = input[1].type = input[2].type = input[3].type = INPUT_KEYBOARD;
	input[0].ki.wVk = input[3].ki.wVk = VK_CONTROL;
	input[1].ki.wVk = input[2].ki.wVk = 'V';
	input[2].ki.dwFlags = input[3].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(4, input, sizeof(INPUT));
	Sleep(20);
}

void SendEnter()
{
	INPUT input[2];
	memset(input, 0, sizeof(input));
	input[0].type = input[1].type = INPUT_KEYBOARD;
	input[0].ki.wVk = input[1].ki.wVk = VK_RETURN;
	input[1].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(2, input, sizeof(INPUT));
	Sleep(20);
}

void SendCtlEnter()
{
	INPUT input[4];
	memset(input, 0, sizeof(input));
	input[0].type = input[1].type = input[2].type = input[3].type = INPUT_KEYBOARD;
	input[0].ki.wVk = input[3].ki.wVk = VK_CONTROL;
	input[1].ki.wVk = input[2].ki.wVk = VK_RETURN;
	input[2].ki.dwFlags = input[3].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(4, input, sizeof(INPUT));
	Sleep(20);
}

void SendMouse(int x, int y, int flag = 0)
{
	SetCursorPos(x, y);
	if (flag == 0)
	{
		INPUT input[2];
		memset(input, 0, sizeof(input));
		input[0].type = input[1].type = INPUT_MOUSE;
		input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(2, input, sizeof(INPUT));
	}
	else if (flag == 1)
	{
		INPUT input[2];
		memset(input, 0, sizeof(input));
		input[0].type = input[1].type = INPUT_MOUSE;
		input[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		input[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		SendInput(2, input, sizeof(INPUT));
	}
	else if (flag == 2)
	{
		INPUT input[4];
		memset(input, 0, sizeof(input));
		input[0].type = input[1].type = input[2].type = input[3].type = INPUT_MOUSE;
		input[0].mi.dwFlags = input[2].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		input[1].mi.dwFlags = input[3].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(4, input, sizeof(INPUT));
	}
	Sleep(20);
}

void SendClean()
{
	INPUT input[6];
	memset(input, 0, sizeof(input));
	input[0].type = input[1].type = input[2].type = input[3].type = input[4].type = input[5].type = INPUT_KEYBOARD;
	input[0].ki.wVk = input[2].ki.wVk = VK_CONTROL;
	input[1].ki.wVk = input[3].ki.wVk = 65;
	input[4].ki.wVk = input[5].ki.wVk = VK_BACK;
	input[2].ki.dwFlags = input[3].ki.dwFlags = input[5].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(6, input, sizeof(INPUT));
	Sleep(20);
}

void SetFilesToClipboard(const std::string sFile)
{
	DROPFILES dobj = { 20, { 0, 0 }, 0, 1 };
	int nLen = (int)sFile.length() + 1;
	int nGblLen = sizeof(dobj) + nLen * 2 + 5;//lots of nulls and multibyte_char
	HGLOBAL hGbl = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, nGblLen);
	char* sData = (char*)::GlobalLock(hGbl);
	memcpy(sData, &dobj, 20);
	char* sWStr = sData + 20;
	for (int i = 0; i < nLen * 2; i += 2)
	{
		sWStr[i] = sFile[i / 2];
	}
	::GlobalUnlock(hGbl);
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		SetClipboardData(CF_HDROP, hGbl);
		CloseClipboard();
	}
}

void SetAsniToClipBoard(const std::string text)
{
	if (OpenClipboard(NULL))
	{
		HGLOBAL clipbuffer;
		char * buffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, text.length() + 1);
		buffer = (char*)GlobalLock(clipbuffer);
		strcpy_s(buffer, text.length() + 1, text.c_str());
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_TEXT, clipbuffer);
		CloseClipboard();
	}
}

std::string GetAsniFromClipBoard()
{
	// Try opening the clipboard
	if (!OpenClipboard(nullptr))
		return "";

	// Get handle of clipboard object for ANSI text
	HANDLE hData = GetClipboardData(CF_TEXT);
	if (hData == nullptr)
		return "";

	// Lock the handle to get the actual text pointer
	char * pszText = static_cast<char*>(GlobalLock(hData));
	if (pszText == nullptr)
		return "";

	// Save text in a string class instance
	std::string text(pszText);

	// Release the lock
	GlobalUnlock(hData);

	// Release the clipboard
	CloseClipboard();

	return text;
}

Mat GetScreenForm() {
	while (!IsWindow(hq))
	{
		hq = FindWindow("WeChatMainWndForPC", NULL);
	}
	if (IsIconic(hq))
	{
		SetWindowPos(hq, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_ASYNCWINDOWPOS);
		SetForegroundWindow(hq);
		Sleep(20);
	}

	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	Mat res;
	BITMAPINFOHEADER  bi;

	hwindowDC = GetDC(hq);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	//RECT windowsize;    // get the height and width of the screen
	GetClientRect(hq, &windowsize);

	srcheight = windowsize.bottom;
	srcwidth = windowsize.right;
	height = windowsize.bottom;  //change this to whatever size you want to resize to
	width = windowsize.right;

	res.create(height, width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, res.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

	// avoid memory leak
	DeleteObject(hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(hq, hwindowDC);

	Mat resGray(height, width, CV_8UC1);
	for (int nRow = 0; nRow < height; nRow++)
	{
		uchar *srcRow = (res.ptr<uchar>(nRow));
		uchar *pucRow = (resGray.ptr<uchar>(nRow));
		for (int nCol = 0; nCol < width; nCol++)
		{
			pucRow[nCol] = srcRow[nCol * 4 + 1];
		}
	}

	return resGray;
}

std::string WeGetMessage(const Point content, const int left, const int top)
{
	SetWindowPos(hq, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_ASYNCWINDOWPOS);
	SetForegroundWindow(hq);
	Sleep(20);

	RECT rect;
	GetWindowRect(hq, &rect);

	SendMouse(rect.left + content.x, rect.top + content.y, 1);
	SendMouse(rect.left + content.x + 4, rect.top + content.y + 4, 0);
	SetCursorPos(rect.left, rect.top);
	return GetAsniFromClipBoard();
}

void WeSendText(const int left, const int top, const std::string text)
{
	SetWindowPos(hq, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_ASYNCWINDOWPOS);
	SetForegroundWindow(hq);
	Sleep(20);

	RECT rect;
	GetWindowRect(hq, &rect);

	SetAsniToClipBoard(text);
	//SendMouse(rect.left + left + 64, rect.top + top + 64, 1);
	//Sleep(20);
	//SendMouse(rect.left + left + 64 + 4, rect.top + top + 64 + 4, 0);
	//Sleep(20);
	//SendEnter();
	
	SendMouse(rect.left + left + 64, rect.top + top + 64, 0);
	SendCtlV();
	SendEnter();
}

void WeSendFile(const int left, const int top, const std::string text)
{
	SetWindowPos(hq, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_ASYNCWINDOWPOS);
	SetForegroundWindow(hq);
	Sleep(20);

	RECT rect;
	GetWindowRect(hq, &rect);

	SetFilesToClipboard(text);
	//SendMouse(rect.left + left + 64, rect.top + top + 64, 1);
	//Sleep(20);
	//SendMouse(rect.left + left + 64 + 4, rect.top + top + 64 + 4, 0);
	//Sleep(20);
	//SendEnter();
	
	SendMouse(rect.left + left + 64, rect.top + top + 64, 0);
	SendCtlV();
	SendEnter();
}

void WeSendMessage(const int left, const int top, const std::string msg)
{
	SetWindowPos(hq, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_ASYNCWINDOWPOS);
	SetForegroundWindow(hq);
	Sleep(20);

	pair<bool, std::string> tmpStr;
	vector<pair<bool, std::string>> context;
	for (int i = 0; i <= msg.length(); i++)
	{
		if (msg[i] == '\f' || i == msg.length())
		{
			if (tmpStr.second.length() > 0)
				context.push_back(tmpStr);
			tmpStr.first = !tmpStr.first;
			tmpStr.second = "";
		}
		else
			tmpStr.second.push_back(msg[i]);
	}

	for (int i = 0; i < context.size(); i++)
	{
		if (!context[i].first)
		{
			WeSendText(left, top, context[i].second);
		}
		else
		{
			WeSendFile(left, top, context[i].second);
		}
	}

}

std::string GetCommand()
{
	if (!outDate)
	{
		outDate = true;
		return commandStr;
	}
	else
		return "";
}

void TaskUpdateFormFunc() {

	curImg = GetScreenForm();
	if (!outDate || !commandFinish)
	{
		preImg = curImg;
		return;
	}
	if (preImg.cols != curImg.cols || preImg.rows != curImg.rows)
	{
		preImg = curImg;
	}
	else
	{
		//int left, top;
		for (int i = preImg.cols - 4; i > 0; i--)
		{
			if (preImg.at<uchar>(preImg.rows - 4, i) != preImg.at<uchar>(preImg.rows - 4, i - 1))
			{
				mleft = i;
				break;
			}
		}
		for (int j = preImg.rows - 4; j > 0; j--)
		{
			if (preImg.at<uchar>(j, preImg.cols - 4) != preImg.at<uchar>(j - 1, preImg.cols - 4))
			{
				mtop = j;
				break;
			}
		}
		bool diff = false;
		for (int i = mleft; i < mleft + 2 * OFFSET; i++)
		{
			for (int j = mtop - 2; j > mtop / 3; j--)
			{
				if (preImg.at<uchar>(j, i) != curImg.at<uchar>(j, i))
				{
					diff = true;
					break;
				}
			}
		}
		if (!diff)
			return;

		Sleep(20);
		preImg = GetScreenForm();
		bool command = false;
		Point content = Point(0, 0);
		for (int i = mtop - 2; i > 0; i--)
		{
			if (preImg.at<uchar>(i, mleft + OFFSET) != preImg.at<uchar>(i - 1, mleft + OFFSET))
			{
				command = true;
				content.x = mleft + 104;
				content.y = i - 18;
				break;
			}
			if (preImg.at<uchar>(i, preImg.cols - OFFSET) != preImg.at<uchar>(i - 1, preImg.cols - OFFSET))
			{
				command = false;
				content.x = preImg.cols - 104;
				content.y = i - 18;
				break;
			}
		}
		if (command)
		{
			commandStr = WeGetMessage(content, mleft, mtop);
			cout << commandStr << endl;
			outDate = false;
			//commandStr = "kiv:\fE:/jenny.jpg\fend";
			//WeSendMessage(left, top, commandStr);
		}
	}
}

std::string fxread(tcp::socket &socket)
{
	asio::streambuf buf;
	asio::error_code ignored_error;
	asio::read(socket, buf, asio::transfer_exactly(4), ignored_error);
	int tranSize = *asio::buffer_cast<const int*>(buf.data());
	buf.consume(4);
	asio::read(socket, buf, asio::transfer_exactly(tranSize), ignored_error);
	std::string str(asio::buffers_begin(buf.data()), asio::buffers_end(buf.data()));
	return str;
}

void fxwrite(tcp::socket &socket, const std::string &vec)
{
	asio::error_code ignored_error;
	std::vector<char> buff(vec.size() + 4);
	*(int*)buff.data() = (int)vec.size();
	for (int i = 0; i < vec.size(); i++)
	{
		buff[i + 4] = vec[i];
	}
	asio::write(socket, asio::buffer(buff), asio::transfer_exactly(buff.size()), ignored_error);
}

int main()
{

	Timer TaskUpdateForm;
	TaskUpdateForm.StartTimer(100, std::bind(TaskUpdateFormFunc));
	
	for (;;)
	{
		try
		{
			asio::io_context io_context;
			tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 34580));

			for (;;)
			{
				tcp::socket socket(io_context);
				acceptor.accept(socket);

				std::string str(fxread(socket));
				if (str == "#GETCOMMAND#")
				{
					fxwrite(socket, GetCommand());
				}
				else
				{
					commandFinish = false;
					outDate = true;
					WeSendMessage(mleft, mtop, str);
					commandFinish = true;
					fxwrite(socket, "#OK#");
				}
			}
		}
		catch (std::exception& e)
		{
			cout << e.what() << endl;
		}
	}

	std::this_thread::sleep_for(std::chrono::seconds(36000000));
	return 0;

	while (1)
	{
		if (GetKeyState(VK_F2) & 0x8000)
		{
			//SendCtlC();
			//SendCtlV();
			SendEnter();
			Sleep(400);
		}

	}
	return 0;
}
