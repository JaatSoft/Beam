/*
	BmJobStatusWin.h
	
		$Id$
*/

#ifndef _BmJobStatusWin_h
#define _BmJobStatusWin_h

#include <map>

#include <MBorder.h>
#include <MWindow.h>
#include <VGroup.h>

#include "BmController.h"

class BmPopAccount;
class BmPopper;

/*------------------------------------------------------------------------------*\
	BmJobStatusView
		-	controls a specific connection
\*------------------------------------------------------------------------------*/
class BmJobStatusView : public MBorder, public BmJobController {

public:

	// c'tors and d'tor:
	BmJobStatusView( const char* name);
	virtual ~BmJobStatusView();

	// native methods:
	virtual bool WantsToStayVisible()	{ return false; }
	virtual void ResetController()		{ }
	virtual void UpdateModelView( BMessage* msg) = 0;
	virtual BmJobModel* CreateJobModel( BMessage* msg) = 0;

	// overrides of controller base:
	BHandler* GetControllerHandler() 	{ return this; }
	virtual void JobIsDone( bool completed);

	// overrides of view base:
	virtual void MessageReceived( BMessage* msg);
};

/*------------------------------------------------------------------------------*\
	BmPopperView
		-	controls a specific POP3-connection
\*------------------------------------------------------------------------------*/
class BmPopperView : public BmJobStatusView {

public:
	// creator-func, c'tors and d'tor:
	static BmPopperView* CreateInstance( const char* name);
	BmPopperView( const char* name);
	~BmPopperView();

	// native methods:
	BmJobModel* CreateJobModel( BMessage* msg);

	// overrides of connectionview base:
	bool WantsToStayVisible();
	void ResetController();
	void UpdateModelView( BMessage* msg);

	// overrides of controller base:
	BHandler* GetControllerHandler() 	{ return this; }

private:
	BStatusBar* mStatBar;				// shows current status of this connection
	BStatusBar* mMailBar;				// shows number of mails handled by this connection

};

/*------------------------------------------------------------------------------*\
	BmJobStatusWin
		-	implements the connection-window, where the states of all active connections
			are displayed
		-	three different display-modes are available:
			* DYNAMIC:			JobStatuss are only displayed as long as they are active,
									when no more connections is active, the window will close.
			* DYNAMIC_EMPTY:	Only POP-connections that actually received mail stay visible
									after the connection has ended
			* STATIC:			All connections are visible, even after connection-close
		-	in general each connection to any server starts as a request to this class (better: the
			one and only instance of this class). For every requested connection a new
			interface is generated and displayed, then the connections is executed by the
			corresponding connection-object (for instance a BmPopper).
		-	connections and their interfaces are connected via the interface defined between
			BmJobController and BmJobModel.
\*------------------------------------------------------------------------------*/
class BmJobStatusWin : public MWindow {
	typedef MWindow inherited;

	friend class BmJobStatusView;

	static const uint32 MyWinFlags = B_ASYNCHRONOUS_CONTROLS
												|	B_NOT_ZOOMABLE
												|	B_NOT_RESIZABLE;

	typedef map<BString, BmJobStatusView*> JobStatusMap;

public:
	// c'tors and d'tor:
	BmJobStatusWin( const char* title, BLooper* invoker=NULL);
	virtual ~BmJobStatusWin();

	// overrides of BWindow base:
	bool QuitRequested();
	void Quit();
	virtual void MessageReceived( BMessage* msg);

	//
	static const rgb_color BM_COL_STATUSBAR;

	//	
	static BmJobStatusWin* Instance;

	//	message component definitions:
	static const char* const MSG_CONN_NAME = 		"bm:connname";
	static const char* const MSG_CONN_INFO = 		"bm:conninfo";

private:
	void AddJobStatus( BMessage* msg);
	void RemoveJobStatus( const char* name);

	JobStatusMap mActiveJobStatuss;	// list of known connections (some may be inactive)
	VGroup* mOuterGroup;						// the outmost view that the connection-interfaces live in
	BLooper* mInvokingLooper;				// the looper we will tell that we are finished
	uint8 mActiveConnCount;					// number of currently active connections

};

#endif
