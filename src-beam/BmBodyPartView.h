/*
	BmBodyPartView.h
		$Id$
*/

#ifndef _BmBodyPartView_h
#define _BmBodyPartView_h

#include <map>

#include "BmListController.h"

/*------------------------------------------------------------------------------*\
	BmBodyPartItem
		-	
\*------------------------------------------------------------------------------*/
class BmBodyPartItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	BmBodyPartItem( BString key, BmListModelItem* item);
	~BmBodyPartItem();
};

class BmBodyPartList;
class BmMailView;
/*------------------------------------------------------------------------------*\
	BmBodyPartView
		-	
\*------------------------------------------------------------------------------*/
class BmBodyPartView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	// c'tors and d'tor:
	BmBodyPartView( minimax minmax, int32 width, int32 height);
	~BmBodyPartView();

	// native methods:
	void ShowBody( BmBodyPartList* body);
	void ShowMenu( BPoint point);

	// overrides of listview base:
	CLVContainerView* CreateContainer( bool horizontal, bool vertical, 
												  bool scroll_view_corner, border_style border, 
												  uint32 ResizingMode, uint32 flags);
	bool InitiateDrag( BPoint point, int32 index, bool wasSelected);
	void MessageReceived( BMessage* msg);
	void MouseDown( BPoint point);

	// overrides of controller base:
	void AddAllModelItems();
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	void ItemInvoked( int32 index);
	BString StateInfoBasename();

	// getters:
	float FixedWidth() 						{ return 5000; }

	static const int16 nFirstTextCol;

private:
	static float nColWidths[10];
	float mColWidths[10];
	bool mShowAllParts;
};

#endif