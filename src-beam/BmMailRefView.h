/*
	BmMailRefView.h
		$Id$
*/

#ifndef _BmMailRefView_h
#define _BmMailRefView_h

#include <map>

#include "BmListController.h"

class BmMailFolder;

/*------------------------------------------------------------------------------*\
	BmMailRefItem
		-	
\*------------------------------------------------------------------------------*/
class BmMailRefItem : public BmListViewItem
{
	typedef BmListViewItem inherited;
	static const int16 nFirstTextCol;

public:
	// c'tors and d'tor:
	BmMailRefItem( BString key, BmListModelItem* item);
	~BmMailRefItem();
	
	// overrides of CLVEasyItem base:
	const int32 GetNumValueForColumn( int32 column_index) const;
	const time_t GetDateValueForColumn( int32 column_index) const;

};

/*------------------------------------------------------------------------------*\
	BmMailRefView
		-	
\*------------------------------------------------------------------------------*/
class BmMailRefView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	//
	static BmMailRefView* CreateInstance( minimax minmax, int32 width, int32 height);
	
	// c'tors and d'tor:
	BmMailRefView( minimax minmax, int32 width, int32 height);
	~BmMailRefView();

	// native methods:
	void ShowFolder( BmMailFolder* folder);

	// overrides of listview base:
	void MessageReceived( BMessage* msg);

	// overrides of controller base:
	BString StateInfoBasename();
	BMessage* DefaultLayout();
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	
private:
	BmMailFolder* mCurrFolder;

};


#endif
