#include "../../global.h"
#include "../../RageUtil.h"

#include "LoadingWindow_Win32.h"
#include "../../resource.h"
#include <windows.h>

HBITMAP g_hBitmap = NULL;


BOOL CALLBACK LoadingWindow_Win32::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	AppInstance handle;

	switch( msg )
	{
	case WM_INITDIALOG:
		g_hBitmap = 
			(HBITMAP)LoadImage( 
				handle.Get(), 
				"Data\\splash.bmp",
				IMAGE_BITMAP,
				0, 0,
				LR_LOADFROMFILE );
		break;
	case WM_PAINT:
		{
			if( g_hBitmap )
			{
				PAINTSTRUCT ps;
				HDC hdcDst = BeginPaint( hWnd, &ps );
				HDC hdcSrc = CreateCompatibleDC( NULL );
				SelectObject( hdcSrc, g_hBitmap );
				BOOL bSuccess = BitBlt(
					hdcDst,
					0, 0,
					1000, 1000,		// let GDI do the clipping...
					hdcSrc,
					0, 0,
					SRCCOPY );
//				DWORD dwLastError = GetLastError();
				ASSERT( bSuccess );
				EndPaint( hWnd, &ps );
			}
			return FALSE;
		}
		break;
	case WM_DESTROY:
		DeleteObject( g_hBitmap );
		g_hBitmap = NULL;
		break;
	}

	return FALSE;
}

LoadingWindow_Win32::LoadingWindow_Win32()
{
	hwnd = CreateDialog(handle.Get(), MAKEINTRESOURCE(IDD_LOADING_DIALOG), NULL, WndProc);

	SetText("Initializing hardware...");
	Paint();
}

LoadingWindow_Win32::~LoadingWindow_Win32()
{
	if(hwnd)
		DestroyWindow( hwnd );
}

void LoadingWindow_Win32::Paint()
{
	SendMessage( hwnd, WM_PAINT, 0, 0 );

	/* Process all queued messages since the last paint.  This allows the window to
	 * come back if it loses focus during load. */
	MSG msg;
	while( PeekMessage( &msg, hwnd, 0, 0, PM_NOREMOVE ) ) {
		GetMessage(&msg, hwnd, 0, 0 );
		DispatchMessage( &msg );
	}
}

void LoadingWindow_Win32::SetText(CString str)
{
	CStringArray asMessageLines;
	split( str, "\n", asMessageLines, false );

	SendDlgItemMessage( hwnd, IDC_STATIC_MESSAGE1, WM_SETTEXT, 0, 
		(LPARAM)(LPCTSTR)asMessageLines[0]);
	SendDlgItemMessage( hwnd, IDC_STATIC_MESSAGE2, WM_SETTEXT, 0, 
		(LPARAM)(LPCTSTR)(asMessageLines.size()>=2 ? asMessageLines[1] : ""));
	SendDlgItemMessage( hwnd, IDC_STATIC_MESSAGE3, WM_SETTEXT, 0, 
		(LPARAM)(LPCTSTR)(asMessageLines.size()>=3 ? asMessageLines[2] : ""));
}

/*
 * Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
 *
 * Chris Danford
 * Glenn Maynard
 */
