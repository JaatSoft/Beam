/*
	BmMailView.h
		$Id$
*/

#ifndef _BmMailView_h
#define _BmMailView_h

#include <layout.h>

#include "BetterScrollView.h"
#include "Colors.h"
#include "WrappingTextView.h"

#include "BmController.h"
#include "BmMail.h"

class BmBodyPart;
class BmBodyPartView;
class BmMailRef;
class BmMailViewContainer;
class BmMailHeaderView;
class BmMailRefView;
class BmRulerView;

/*------------------------------------------------------------------------------*\
	types of messages sent via the observe/notify system:
\*------------------------------------------------------------------------------*/
#define BM_NTFY_MAIL_VIEW	'bmbc'
						// sent from BmMailView to observers whenever selection changes

#define BM_MAILVIEW_SHOWRAW						'bmMa'
#define BM_MAILVIEW_SHOWCOOKED					'bmMb'
#define BM_MAILVIEW_SHOWINLINES_SEPARATELY	'bmMc'
#define BM_MAILVIEW_SHOWINLINES_CONCATENATED	'bmMd'

/*------------------------------------------------------------------------------*\
	BmMailView
		-	
\*------------------------------------------------------------------------------*/
class BmMailView : public WrappingTextView, public BmJobController {
	typedef WrappingTextView inherited;
	typedef BmJobController inheritedController;
	struct BmTextRunInfo {
		BmTextRunInfo( rgb_color c=Black, bool url=false, BFont f=*be_fixed_font) { 
			color = c; isURL = url; font = f;
		}
		rgb_color color;
		bool isURL;
		BFont font;
	};
	typedef map<int32,BmTextRunInfo> BmTextRunMap;
	typedef BmTextRunMap::const_iterator BmTextRunIter;
	
	// archival-fieldnames:
	static const char* const MSG_RAW = 			"bm:raw";
	static const char* const MSG_FONTNAME = 	"bm:fnt";
	static const char* const MSG_FONTSIZE =	"bm:fntsz";

public:
	static const char* const MSG_HAS_MAIL = 		"bm:hmail";

	// creator-func, c'tors and d'tor:
	static BmMailView* CreateInstance(  minimax minmax, BRect frame, bool outbound);
	BmMailView( minimax minmax, BRect frame, bool outbound);
	~BmMailView();

	// native methods:
	void ShowMail( BmMailRef* ref, bool async=true);
	void ShowMail( BmMail* mail, bool async=true);
	void DisplayBodyPart( BString& displayText, BmBodyPart* bodyPart);
	status_t Archive( BMessage* archive, bool deep=true) const;
	status_t Unarchive( BMessage* archive, bool deep=true);
	bool WriteStateInfo();
	void GetWrappedText( BString& out);
	void LaunchURL( BString url);

	// overrides of BTextView base:
	bool AcceptsDrop( const BMessage* msg);
	void AttachedToWindow();
	void FrameResized( float newWidth, float newHeight);
	void KeyDown(const char *bytes, int32 numBytes);
	void MakeFocus(bool focused);
	void MessageReceived( BMessage* msg);
	void MouseDown( BPoint point);
	void MouseUp( BPoint point);

	// overrides of BmController base:
	BHandler* GetControllerHandler()		{ return this; }
	void JobIsDone( bool completed);
	void DetachModel();

	// getters:
	BmMailViewContainer* ContainerView() const	{ return mScrollView; }
	BmRef<BmMail> CurrMail()				{ return mCurrMail; }
	bool ShowRaw()								{ return mShowRaw; }
	bool ShowInlinesSeparately()			{ return mShowInlinesSeparately; }
	BmBodyPartView* BodyPartView()		{ return mBodyPartView; }

	// setters:
	void TeamUpWith( BmMailRefView* v)	{ mPartnerMailRefView = v; }
	void ShowRaw( bool b) 					{ mShowRaw = b; }
	void ShowInlinesSeparately( bool b) { mShowInlinesSeparately = b; }

private:
	void ShowMenu( BPoint point);
	BmTextRunIter TextRunInfoAt( int32 pos) const;

	// will not be archived:
	bool mOutbound;
	BmRef< BmMail> mCurrMail;
	BmMailViewContainer* mScrollView;
	BmMailHeaderView* mHeaderView;
	BmBodyPartView* mBodyPartView;
	BmRulerView* mRulerView;
	BmMailRefView* mPartnerMailRefView;
	BmTextRunMap mTextRunMap;
	BmTextRunIter mClickedTextRun;
	
	// will be archived:
	BString mFontName;
	int16 mFontSize;
	bool mShowRaw;
	bool mShowInlinesSeparately;

	// Hide copy-constructor and assignment:
	BmMailView( const BmMailView&);
	BmMailView operator=( const BmMailView&);
};

class BmBusyView;
/*------------------------------------------------------------------------------*\
	BmMailViewContainer
		-	
\*------------------------------------------------------------------------------*/
class BmMailViewContainer : public MView, public BetterScrollView {
	typedef BetterScrollView inherited;

public:
	BmMailViewContainer( minimax minmax, BmMailView* target, uint32 resizingMode, 
								uint32 flags);
	~BmMailViewContainer();

	// native methods:
	void SetBusy();
	void UnsetBusy();
	void PulseBusyView();

	// overrides of BView base:
	void FrameResized(float new_width, float new_height);
	void Draw( BRect bounds);

	// additions for liblayout:
	virtual minimax layoutprefs();
	virtual BRect layout(BRect);

private:
	BmBusyView* mBusyView;

	// Hide copy-constructor and assignment:
	BmMailViewContainer( const BmMailViewContainer&);
	BmMailViewContainer operator=( const BmMailViewContainer&);
};

#endif
