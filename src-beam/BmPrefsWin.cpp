/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <Autolock.h>
#include <MenuField.h>
#include <Font.h>
#include <Message.h>
#include "BmString.h"

#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
	class BFont;
	class BMessage;
	class BPopUpMenu;
	class BRect;
#endif

#include <HGroup.h>
#include <LayeredGroup.h>
#include <Space.h>
#include <VGroup.h>

#include "BetterButton.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"
#include "UserResizeSplitView.h"

#include "BeamApp.h"
#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmPrefsView.h"
#include "BmPrefsFilterView.h"
#include "BmPrefsFilterChainView.h"
#include "BmPrefsGeneralView.h"
#include "BmPrefsIdentityView.h"
#include "BmPrefsLoggingView.h"
#include "BmPrefsMailConstrView.h"
#include "BmPrefsMailReadView.h"
#include "BmPrefsRecvMailView.h"
#include "BmPrefsSendMailView.h"
#include "BmPrefsShortcutsView.h"
#include "BmPrefsSignatureView.h"
#include "BmResources.h"
#include "BmPrefsWin.h"
#include "BmUtil.h"



/********************************************************************************\
	BmPrefsWin
\********************************************************************************/

enum {
	BM_SAVE_CHANGES    = 'bmWS',
	BM_REVERT_CHANGES  = 'bmWR',
	BM_SET_DEFAULTS    = 'bmWD'
};

BmPrefsWin* BmPrefsWin::theInstance = NULL;

const char* const BmPrefsWin::MSG_VSPLITTER = 	"bm:vspl";

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates the app's main window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmPrefsWin* BmPrefsWin::CreateInstance() 
{
	if (!theInstance) {
		theInstance = new BmPrefsWin;
		theInstance->ReadStateInfo();
	}
	return theInstance;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsWin::BmPrefsWin()
	:	inherited( "PrefsWin", BRect(50,50,849,449),
					  "Beam Preferences",
					  B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 
					  B_ASYNCHRONOUS_CONTROLS)
	,	mPrefsListView( NULL)
	,	mPrefsViewContainer( NULL)
	,	mVertSplitter( NULL)
	,	mChanged( false)
{
	MView* mOuterGroup = 
		new VGroup( 
			minimax( -1, 550),
			new MBorder( M_RAISED_BORDER, 5, NULL,
				new HGroup(
					minimax(-1,200),
					mVertSplitter = new UserResizeSplitView(
						new VGroup(
							minimax(100,200,200,1E5),
							new BetterScrollView( 
								minimax(100,200,200,1E5), 
								CreatePrefsListView( 120, 200),
								BM_SV_V_SCROLLBAR
							),
							new Space( minimax(0,2,0,2)),
							0
						),
						mPrefsViewContainer = new BmPrefsViewContainer(
							new LayeredGroup( 
								new BmPrefsView( NULL),
								new BmPrefsGeneralView(),
								new BmPrefsShortcutsView(),
								new BmPrefsLoggingView(),
								new BmPrefsMailConstrView(),
								new BmPrefsSendMailView(),
								new BmPrefsMailReadView(),
								new BmPrefsRecvMailView(),
								new BmPrefsIdentityView(),
								new BmPrefsSignatureView(),
								new BmPrefsFilterView(),
								new BmPrefsFilterChainView(),
								0
							)
						),
						"vsplitter", 150, B_VERTICAL, true, true, false, false, 
						false, B_FOLLOW_NONE
					),
					0
				)
			),
			new Space( minimax(0,10,0,10)),
			new HGroup(
				new Space(),
				mDefaultsButton = new BetterButton( "Defaults", new BMessage(BM_SET_DEFAULTS), this, minimax(-1,-1,-1,-1)),
				new Space( minimax(40,0,40,0)),
				mRevertButton = new BetterButton( "Revert", new BMessage(BM_REVERT_CHANGES), this, minimax(-1,-1,-1,-1)),
				new Space( minimax(20,0,20,0)),
				mSaveButton = new BetterButton( "Save", new BMessage(BM_SAVE_CHANGES), this, minimax(-1,-1,-1,-1)),
				new Space( minimax(20,0,20,0)),
				0
			),
			new Space( minimax(0,10,0,10)),
			0
		);
	AddChild( dynamic_cast<BView*>(mOuterGroup));
	mSaveButton->SetEnabled( false);
	mRevertButton->SetEnabled( false);
	mPrefsListView->Select( 0);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsWin::~BmPrefsWin() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmPrefsWin::ArchiveState( BMessage* archive) const {
	status_t ret = inherited::ArchiveState( archive)
						|| archive->AddFloat( MSG_VSPLITTER, mVertSplitter->DividerLeftOrTop());
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmPrefsWin::UnarchiveState( BMessage* archive) {
	float vDividerPos;
	status_t ret = inherited::UnarchiveState( archive)
						|| archive->FindFloat( MSG_VSPLITTER, &vDividerPos);
	if (ret == B_OK) {
		mVertSplitter->SetPreferredDividerLeftOrTop( vDividerPos);
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
ColumnListView* BmPrefsWin::CreatePrefsListView( int32 width, int32 height) {
	mPrefsListView = new ColumnListView( 
		BRect( 0, 0, float(width-1), float(height-1)), NULL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
		B_SINGLE_SELECTION_LIST, true, true
	);

	BFont font(*be_bold_font);
	mPrefsListView->SetFont( &font);
	mPrefsListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mPrefsListView->SetTarget( this);
	mPrefsListView->ClickSetsFocus( true);
	mPrefsListView->SetMinItemHeight( 
		MAX( TheResources->FontLineHeight(), 
			  float(ThePrefs->GetInt( "ListviewHierarchicalMinItemHeight", 16)))
	);
	mPrefsListView->AddColumn( 
		new CLVColumn( NULL, 10.0, 
							CLV_EXPANDER | CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE
							| CLV_COLTYPE_BITMAP, 
							10.0));
	mPrefsListView->AddColumn( 
		new CLVColumn( "   Category", 300.0, 
							CLV_NOT_MOVABLE|CLV_NOT_RESIZABLE|CLV_TELL_ITEMS_WIDTH, 300.0));

	CLVEasyItem* item;	
	item = new CLVEasyItem( 0, false, false, mPrefsListView);
	item->SetColumnContent( 1, "General");
	item->SetExpanded( true);
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 1, false, false, mPrefsListView);
	item->SetColumnContent( 1, "Shortcuts");
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 1, false, false, mPrefsListView);
	item->SetColumnContent( 1, "Logging");
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 0, true, false, mPrefsListView);
	item->SetColumnContent( 1, "Sending mail");
	item->SetExpanded( true);
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 1, false, false, mPrefsListView);
	item->SetColumnContent( 1, "Accounts");
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 0, true, false, mPrefsListView);
	item->SetColumnContent( 1, "Receiving mail");
	item->SetExpanded( true);
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 1, false, false, mPrefsListView);
	item->SetColumnContent( 1, "Accounts");
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 0, true, false, mPrefsListView);
	item->SetColumnContent( 1, "Identities");
	item->SetExpanded( true);
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 1, false, false, mPrefsListView);
	item->SetColumnContent( 1, "Signatures");
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 0, true, false, mPrefsListView);
	item->SetColumnContent( 1, "Filtering mail");
	item->SetExpanded( true);
	mPrefsListView->AddItem( item);

	item = new CLVEasyItem( 1, false, false, mPrefsListView);
	item->SetColumnContent( 1, "Chains");
	mPrefsListView->AddItem( item);

	return mPrefsListView;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsWin::SendMsgToSubView( const BmString& subViewName, BMessage* msg) {
	int32 index = 0;
	BmPrefsView* pv = mPrefsViewContainer->ShowPrefsByName( subViewName, index);
	if (pv) {
		mPrefsListView->Select( index);
		PostMessage( msg, pv);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsWin::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_SELECTION_CHANGED: {
				int32 index = mPrefsListView->CurrentSelection( 0);
				int32 fullIndex = mPrefsListView->FullListIndexOf( 
					(CLVListItem*)mPrefsListView->ItemAt( index)
				);
				mPrefsViewContainer->ShowPrefs( 1+fullIndex);
				break;
			}
			case BM_PREFS_CHANGED: {
				mChanged = true;
				mSaveButton->SetEnabled( true);
				mRevertButton->SetEnabled( true);
				break;
			}
			case BM_SAVE_CHANGES: {
				if (mPrefsViewContainer->SaveChanges()) {
					mSaveButton->SetEnabled( false);
					mRevertButton->SetEnabled( false);
					mChanged = false;
				}
				break;
			}
			case BM_REVERT_CHANGES: {
				mPrefsViewContainer->RevertChanges();
				mSaveButton->SetEnabled( false);
				mRevertButton->SetEnabled( false);
				mChanged = false;
				break;
			}
			case BM_SET_DEFAULTS: {
				mPrefsViewContainer->SetDefaults();
				mSaveButton->SetEnabled( true);
				mRevertButton->SetEnabled( true);
				mChanged = true;
				break;
			}
			case BMM_PREFERENCES: {
				BmString subViewName = msg->FindString( "SubViewName");
				if (subViewName.Length()) {
					BMessage subViewMsg;
					if (msg->FindMessage( "SubViewMsg", &subViewMsg) == B_OK)
						SendMsgToSubView( subViewName, &subViewMsg);
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PrefsWin: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmPrefsWin::QuitRequested() {
	BM_LOG2( BM_LogApp, BmString("PrefsWin has been asked to quit"));
	if (mChanged) {
		if (IsMinimized())
			Minimize( false);
		Activate();
		BAlert* alert = new BAlert( "title", "The preferences have changed. Would you like to save the changes before closing?",
											 "Cancel", "Don't save", "Save",
											 B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		alert->SetShortcut( 0, B_ESCAPE);
		int32 result = alert->Go();
		switch( result) {
			case 0:
				return false;
			case 1:
				mPrefsViewContainer->RevertChanges();
				return true;
			case 2:
				return mPrefsViewContainer->SaveChanges();
		}
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmPrefsWin::Quit() {
	mPrefsViewContainer->WriteStateInfo();
	BM_LOG2( BM_LogApp, BmString("PrefsWin has quit"));
	inherited::Quit();
}

/*------------------------------------------------------------------------------*\
	PrefsListSelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsWin::PrefsListSelectionChanged( int32) {
}
