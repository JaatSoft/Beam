/*
	BmApp.h
		$Id$
*/

#ifndef _BmApp_h
#define _BmApp_h

#include <Application.h>
#include <Bitmap.h>
#include <Font.h>
#include <Path.h>
#include <String.h>
#include <Volume.h>

class BmPrefs;
class BmLogHandler;
class BmMailFolderList;
class BmMailFolderView;
class BmMailRefView;
class BmMainWindow;

class BmApplication : public BApplication
{
	typedef BApplication inherited;

public:
	//
	BmApplication( const char *sig);
	~BmApplication();
	// beos-stuff
	bool QuitRequested();
	//
	bool IsQuitting()							{ return mIsQuitting; }

	//
	BmLogHandler* LogHandler;
	BmPrefs* Prefs;
	BBitmap* FolderIcon;
	BBitmap* MailIcon;
	BString HomePath;
	BVolume MailboxVolume;
	BPath	SettingsPath;
	BmMailFolderList* MailFolderList;
	BmMailFolderView* MailFolderView;
	BmMailRefView* MailRefView;
	BmMainWindow* MainWindow;

	//
	font_height BePlainFontHeight;
	float FontHeight();
	float FontLineHeight();
	
	//
	const char* WHITESPACE;

private:
	static int InstanceCount;
	bool mIsQuitting;

};

extern BmApplication* bmApp;

#endif