/*
	BmMainWindow.h
		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


#ifndef _BmMainWindow_h
#define _BmMainWindow_h

#include <MMenuBar.h>

#include "BmController.h"
#include "BmWindow.h"

class BMenu;

class BmMailFolderView;
class BmMailRefView;
class BmMailView;
class BmMailViewContainer;
class BmToolbarButton;
class CLVContainerView;
class UserResizeSplitView;

class BmMainMenuBar : public MMenuBar, public BmJobController
{
	typedef MMenuBar inherited;
	typedef BmJobController inheritedController;

public:

	BmMainMenuBar();

	void MessageReceived( BMessage* msg);

	BHandler* GetControllerHandler() 	{ return this; }
	void JobIsDone( bool completed);

	inline void SetAccountMenu( BMenu* m)	{ mAccountMenu = m; }
	
private:
	BMenu* mAccountMenu;

	// Hide copy-constructor and assignment:
	BmMainMenuBar( const BmMainMenuBar&);
	BmMainMenuBar operator=( const BmMainMenuBar&);
};



class BmMainWindow : public BmWindow
{
	typedef BmWindow inherited;

	// archival-fieldnames:
	static const char* const MSG_VSPLITTER = 	"bm:vspl";
	static const char* const MSG_HSPLITTER = 	"bm:hspl";

public:
	// creator-func, c'tors and d'tor:
	static BmMainWindow* CreateInstance();
	BmMainWindow();
	~BmMainWindow();

	// class-methods:
	static bool IsAlive();

	// native methods:
	void BeginLife();

	// overrides of BWindow base:
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();
	void WorkspacesChanged( uint32 oldWorkspaces, uint32 newWorkspaces);

	static BmMainWindow* theInstance;

protected:
	status_t ArchiveState( BMessage* archive) const;
	status_t UnarchiveState( BMessage* archive);

private:
	CLVContainerView* CreateMailFolderView( minimax minmax, int32 width, int32 height);
	CLVContainerView* CreateMailRefView( minimax minmax, int32 width, int32 height);
	BmMailViewContainer* CreateMailView( minimax minmax, BRect frame);
	MMenuBar* CreateMenu();
	void MailFolderSelectionChanged( int32 numSelected);
	void MailRefSelectionChanged( int32 numSelected);
	void MailViewChanged( bool hasMail);

	BmMailFolderView* mMailFolderView;
	BmMailRefView* mMailRefView;
	BmMailView* mMailView;
	UserResizeSplitView* mVertSplitter;
	UserResizeSplitView* mHorzSplitter;
	
	BmToolbarButton* mCheckButton;
	BmToolbarButton* mNewButton;
	BmToolbarButton* mReplyButton;
	BmToolbarButton* mReplyAllButton;
	BmToolbarButton* mForwardButton;
	BmToolbarButton* mRedirectButton;
	BmToolbarButton* mPrintButton;
	BmToolbarButton* mTrashButton;
	
	BmMainMenuBar* mMainMenuBar;

	static bool nIsAlive;

	// Hide copy-constructor and assignment:
	BmMainWindow( const BmMainWindow&);
	BmMainWindow operator=( const BmMainWindow&);
};

#define TheMainWindow BmMainWindow::theInstance

#endif
