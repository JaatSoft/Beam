/*
	BmPrefsView.h
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


#ifndef _BmPrefsView_h
#define _BmPrefsView_h

#include <Message.h>
#include "BmString.h"
#include <View.h>

#include <layout.h>
#include <MBorder.h>

enum {
	BM_SELECTION_CHANGED 		= 'bm_S',
	BM_COMPLAIN_ABOUT_FIELD 	= 'bm_C',
	BM_PREFS_CHANGED 				= 'bm_!'
};

class MStringView;
/*------------------------------------------------------------------------------*\
	BmPrefsView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsView : public MBorder {
	typedef MBorder inherited;

public:
	// c'tors and d'tor:
	BmPrefsView( BmString label);
	virtual ~BmPrefsView();

	// native methods:
	virtual void Activated();
	virtual void Initialize();
	virtual void Update()					{}
	virtual void SaveData()					{}
	virtual void UndoChanges()				{}
	virtual void SetDefaults()				{}
	virtual bool SanityCheck()				{ return true; }
	virtual void WriteStateInfo()			{}

	void NoticeChange();
	void ResetChanged()						{ mChanged = false; }

	// overrides of BView base:
	void MessageReceived( BMessage* msg);

	// getters:
	bool InitDone() const					{ return mInitDone; }
	const char* Name();

	// message-fields:
	static const char* const MSG_ITEM;
	static const char* const MSG_FIELD_NAME;
	static const char* const MSG_COMPLAINT;

protected:
	BView* mGroupView;
	bool mChanged;

private:
	MStringView* mLabelView;
	bool mInitDone;
	
	// Hide copy-constructor and assignment:
	BmPrefsView( const BmPrefsView&);
	BmPrefsView operator=( const BmPrefsView&);
};



/*------------------------------------------------------------------------------*\
	BmPrefsViewContainer
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsViewContainer : public MBorder
{

public:
	BmPrefsViewContainer( LayeredGroup* group);
	virtual ~BmPrefsViewContainer() 				{}
	
	// native methods:
	void ShowPrefs( int index);
	void WriteStateInfo();
	bool ApplyChanges();
	void RevertChanges();
	void SetDefaults();
	BmPrefsView* ShowPrefsByName( const BmString name);

private:
	LayeredGroup* mLayeredGroup;

};

#endif
