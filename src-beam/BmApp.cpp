/*
	BmApp.cpp
		$Id$
*/

#include <AppFileInfo.h>
#include <Roster.h>
#include <Screen.h>

#include "BmApp.h"
#include "BmBasics.h"
#include "BmDataModel.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

int BmApplication::InstanceCount = 0;

BmApplication* bmApp = NULL;

/*------------------------------------------------------------------------------*\
	BmApplication()
		-	constructor
\*------------------------------------------------------------------------------*/
BmApplication::BmApplication( const char* sig)
	:	inherited( sig)
	,	mIsQuitting( false)
{
	if (InstanceCount > 0)
		throw BM_runtime_error("Trying to initialize more than one instance of class Beam");

	bmApp = this;

	try {
		BmAppName = bmApp->Name();
		// set version info:
		app_info appInfo;
		BFile appFile;
		version_info vInfo;
		BAppFileInfo appFileInfo;
		bmApp->GetAppInfo( &appInfo); 
		appFile.SetTo( &appInfo.ref, B_READ_ONLY);
		appFileInfo.SetTo( &appFile);
		if (appFileInfo.GetVersionInfo( &vInfo, B_APP_VERSION_KIND) == B_OK) {
			BmAppVersion = vInfo.short_info;
		}
		BmAppNameWithVersion = BmAppName + " " + BmAppVersion;

		// create the log-handler:
		BmLogHandler::CreateInstance( 1);

		// load/determine all needed resources:
		BmResources::CreateInstance();

		// load the preferences set by user (if any):
		BmPrefs::CreateInstance();
		
		// create the node-monitor looper:
		BmNodeMonitor::CreateInstance();

		// create the job status window:
		BmJobStatusWin::CreateInstance();
		TheJobStatusWin->Hide();
		TheJobStatusWin->Show();

		InstanceCount++;
	} catch (exception& err) {
		ShowAlert( err.what());
		exit( 10);
	}
}

/*------------------------------------------------------------------------------*\
	~BmApplication()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmApplication::~BmApplication()
{
	BM_PrintRefsLeft();
	delete ThePrefs;
	delete TheResources;
	delete TheLogHandler;
	InstanceCount--;
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmApplication::QuitRequested() {
	mIsQuitting = true;
	TheNodeMonitor->LockLooper();
	bool shouldQuit = inherited::QuitRequested();
	if (!shouldQuit) {
		mIsQuitting = false;
		TheNodeMonitor->UnlockLooper();
	} else
		TheNodeMonitor->Quit();
	return shouldQuit;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( )
		-	
\*------------------------------------------------------------------------------*/
void BmApplication::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
				break;
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("BmApp: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( )
		-	
\*------------------------------------------------------------------------------*/
BRect BmApplication::ScreenFrame() {
	BScreen screen;
	if (!screen.IsValid())
		BM_SHOWERR( BString("Could not initialize BScreen object !?!"));
	return screen.Frame();
}

/*------------------------------------------------------------------------------*\
	HandlesMimetype( )
		-	
\*------------------------------------------------------------------------------*/
bool BmApplication::HandlesMimetype( const BString mimetype) {
	return mimetype.ICompare( "text/x-email")==0 
			 || mimetype.ICompare( "message/rfc822")==0;
}
