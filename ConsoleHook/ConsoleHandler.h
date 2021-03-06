#pragma once

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////

class ConsoleHandler
{
	public:
		ConsoleHandler();
		~ConsoleHandler();

	public:

		DWORD StartMonitorThread();
		void StopMonitorThread();

	private:

		bool OpenSharedObjects();

		void ReadConsoleBuffer();

		void ResizeConsoleWindow(HANDLE hStdOut, DWORD& dwColumns, DWORD& dwRows, DWORD dwResizeWindowEdge);

		void CopyConsoleText();

		void SendConsoleText(HANDLE hStdIn, const wchar_t*	pszText, size_t	textLen);

		void SendMouseEvent(HANDLE hStdIn);

		void ScrollConsole(HANDLE hStdOut, int nXDelta, int nYDelta);

		void SetConsoleParams(DWORD dwHookThreadId, HANDLE hStdOut);

	private:

		static DWORD WINAPI MonitorThreadStatic(LPVOID lpParameter);
		DWORD MonitorThread();

	private:

		SharedMemory<ConsoleParams>       m_consoleParams;
		SharedMemory<ConsoleInfo>         m_consoleInfo;
		SharedMemory<CONSOLE_CURSOR_INFO> m_cursorInfo;
		SharedMemory<CHAR_INFO>           m_consoleBuffer;
		SharedMemory<ConsoleCopy>         m_consoleCopyInfo;
		SharedMemory<MOUSE_EVENT_RECORD>  m_consoleMouseEvent;

		SharedMemory<ConsoleSize>         m_newConsoleSize;
		SharedMemory<SIZE>                m_newScrollPos;

		NamedPipe                         m_consoleMsgPipe;

		std::shared_ptr<void>             m_hMonitorThread;
		std::shared_ptr<void>             m_hMonitorThreadExit;

		DWORD                             m_dwScreenBufferSize;
};

//////////////////////////////////////////////////////////////////////////////


