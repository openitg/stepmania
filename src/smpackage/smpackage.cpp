// smpackage.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "smpackage.h"
#include "smpackageDlg.h"
#include "smpackageInstallDlg.h"
#include "../RageUtil.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSmpackageApp

BEGIN_MESSAGE_MAP(CSmpackageApp, CWinApp)
	//{{AFX_MSG_MAP(CSmpackageApp)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSmpackageApp construction

CSmpackageApp::CSmpackageApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSmpackageApp object

CSmpackageApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSmpackageApp initialization

BOOL CSmpackageApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif


	
	// Make sure the current directory is the root program directory

	// change dir to path of the execuctable
	TCHAR szFullAppPath[MAX_PATH];
	GetModuleFileName(NULL, szFullAppPath, MAX_PATH);
		
	// strip off executable name
	LPSTR pLastBackslash = strrchr(szFullAppPath, '\\');
	*pLastBackslash = '\0';	// terminate the string

	SetCurrentDirectory(szFullAppPath);

	if( !DoesFileExist("Songs") )
	{
		AfxMessageBox( "Your Songs folder could not be located.  Be sure 'smpackage.exe' is in your Stepmania installation folder.", MB_ICONSTOP );
		exit( 1 );
	}
	

	// check if there's a .smzip command line argument
	CStringArray arrayCommandLineBits;
	split( ::GetCommandLine(), "\"", arrayCommandLineBits );
	for( int i=0; i<arrayCommandLineBits.GetSize(); i++ )
	{
		CString sPath = arrayCommandLineBits[i];
		sPath.TrimLeft();
		sPath.TrimRight();
		CString sPathLower = sPath;
		sPathLower.MakeLower();

		// test to see if this is a smzip file
		if( sPathLower.Right(3) == "zip" )
		{
			// We found a zip package.  Prompt the user to install it!
			CSMPackageInstallDlg dlg( sPath );
			int nResponse = dlg.DoModal();
			if( nResponse == IDOK )
			{
				;	// do nothing and fall through to below
			}
			else if (nResponse == IDCANCEL)
			{
				// the user cancelled.  Don't fall through and display the Manager.
				exit(0);
			}
		}
	}


	// Show the Manager Dialog
	CSmpackageDlg dlg;
	int nResponse = dlg.DoModal();
//	if (nResponse == IDOK)


	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
