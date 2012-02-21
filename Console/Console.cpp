// Console.cpp : main source file for Console.exe
//

#include "stdafx.h"

#include "resource.h"

#include "ConsoleView.h"
#include "aboutdlg.h"
#include "MainFrame.h"
#include "Console.h"
#include "JumpList.h"

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Global variables

CAppModule					_Module;

shared_ptr<SettingsHandler>	g_settingsHandler;
shared_ptr<ImageHandler>	g_imageHandler;

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Invisible parent window class for the main window if no taskbar should be shown

class NoTaskbarParent
	: public CWindowImpl<NoTaskbarParent, CWindow, CWinTraits<WS_POPUP, WS_EX_TOOLWINDOW> >
{
	public:
		DECLARE_WND_CLASS(L"NoTaskbarParent")

		NoTaskbarParent() {}
		~NoTaskbarParent() {}

		BEGIN_MSG_MAP(NoTaskbarParent)
		END_MSG_MAP()
};

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////

void ParseCommandLine
(
	LPTSTR lptstrCmdLine, 
	wstring& strConfigFile, 
	bool &bReuse
)
{
	int						argc = 0;
	shared_array<LPWSTR>	argv(::CommandLineToArgvW(lptstrCmdLine, &argc), ::GlobalFree);

	if (argc < 1) return;

	for (int i = 0; i < argc; ++i)
	{
		if (wstring(argv[i]) == wstring(L"-c"))
		{
			// custom config file
			++i;
			if (i == argc) break;
			strConfigFile = argv[i];
		}
		else if (wstring(argv[i]) == wstring(L"-reuse"))
		{
			bReuse = true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////

static bool HandleReuse(LPCTSTR lpstrCmdLine)
{
	SharedMemory<HWND> sharedInstance;
	sharedInstance.Open(L"Console", syncObjNone);
	if (0 != sharedInstance.Get())
	{
		::SetForegroundWindow(*sharedInstance);

		COPYDATASTRUCT cds = {0};
		cds.dwData = 0;
		cds.lpData = (LPVOID)lpstrCmdLine;
		cds.cbData = (_tcslen(lpstrCmdLine) + 1) * sizeof(TCHAR);
		
		::SendMessage(*sharedInstance, WM_COPYDATA, 0, (LPARAM)&cds);

		return true;
	}

	return false;
}

int Run(LPTSTR lpstrCmdLine = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	wstring			strConfigFile(L"");
	bool			bReuse = false;
	bool			bCreateList = false;

	ParseCommandLine(
		lpstrCmdLine, 
		strConfigFile,
		bReuse);

	if (strConfigFile.length() == 0)
	{
		strConfigFile = wstring(L"console.xml");
		bCreateList = true;
//		strConfigFile = Helpers::GetModulePath(NULL) + wstring(L"console.xml");
//		strConfigFile = wstring(::_wgetenv(L"APPDATA")) + wstring(L"\\Console\\console.xml");
	}

	if (!g_settingsHandler->LoadSettings(Helpers::ExpandEnvironmentStrings(strConfigFile)))
	{
		//TODO: error handling
		return 1;
	}

	if (bReuse && HandleReuse(lpstrCmdLine))
		return 0;

	if (bCreateList)
		JumpList::Activate();

	// create main window
	NoTaskbarParent noTaskbarParent;
	MainFrame wndMain(lpstrCmdLine);

	if (!g_settingsHandler->GetAppearanceSettings().stylesSettings.bTaskbarButton)
	{
		noTaskbarParent.Create(NULL);
	}

	if(wndMain.CreateEx(noTaskbarParent.m_hWnd) == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 1;
	}

#ifdef _USE_AERO
	// restore the drop files message in elevated console
	::ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
	::ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
	::ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
#endif

	wndMain.ShowWindow(nCmdShow);

	SharedMemory<HWND> sharedInstance;
	if (bReuse)
	{
		sharedInstance.Create(L"Console", 1, syncObjNone, _T(""));
		sharedInstance = wndMain.m_hWnd;
	}

	int nRet = theLoop.Run();

	if (noTaskbarParent.m_hWnd != NULL) noTaskbarParent.DestroyWindow();

	_Module.RemoveMessageLoop();

	return nRet;
}

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

#ifdef _USE_AERO
  // init GDI+
  ULONG_PTR gdiplusToken;
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
#endif

	g_settingsHandler.reset(new SettingsHandler());
	g_imageHandler.reset(new ImageHandler());

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

#ifdef _USE_AERO
  // shutdown GDI+;
  Gdiplus::GdiplusShutdown(gdiplusToken);
#endif

	_Module.Term();
	g_settingsHandler.reset();

	::CoUninitialize();

	return nRet;
}

//////////////////////////////////////////////////////////////////////////////
