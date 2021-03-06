/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmJobStatusWin_h
#define _BmJobStatusWin_h

#include <deque>
#include <map>

#if defined(B_BEOS_VERSION_DANO) || defined(__HAIKU__)
	class BFont;
	class BMessage;
	class BMessageRunner;
	class BRect;
	class BStatusBar;
#endif
#include <MBorder.h>
#include <VGroup.h>

#include "BmController.h"
#include "BmWindow.h"

using std::deque;
using std::map;

/*------------------------------------------------------------------------------*\
	BmJobStatusView
		-	controls a specific connection
\*------------------------------------------------------------------------------*/
class BmJobStatusView : public MBorder, public BmJobController {
	typedef MBorder inheritedView;
	typedef BmJobController inherited;
	
public:
	// c'tors and d'tor:
	BmJobStatusView( const char* name);
	virtual ~BmJobStatusView();

	// native methods:
	virtual bool AlwaysRemoveWhenDone()	{ return false; }
	virtual void ResetController()		{ }
	virtual void UpdateModelView( BMessage* msg) = 0;
	virtual BmJobModel* CreateJobModel( BMessage* msg) = 0;

	// overrides of controller base:
	void StartJob( BmJobModel* model = NULL, bool startInNewThread=true,
						int32 jobSpecifier = BmJobModel::BM_DEFAULT_JOB);
	BHandler* GetControllerHandler() 	{ return this; }

	// overrides of view base:
	void MessageReceived( BMessage* msg);

	// getters:
	inline int MSecsBeforeShow()			{ return MAX(0,mMSecsBeforeShow); }
	inline int MSecsBeforeRemove()		{ return MAX(0,mMSecsBeforeRemove); }

protected:
	void JobIsDone( bool completed);

	int mMSecsBeforeShow;
	int mMSecsBeforeRemove;
	bool mIsAutoJob;	
							// has this job been invoked automatically?
	
private:
	BMessageRunner* mShowMsgRunner;
	BMessageRunner* mRemoveMsgRunner;
	
	// Hide copy-constructor and assignment:
	BmJobStatusView( const BmJobStatusView&);
#ifndef __POWERPC__
	BmJobStatusView& operator=( const BmJobStatusView&);
#endif
};

class MStringView;
/*------------------------------------------------------------------------------*\
	BmMailMoverView
		-	controls a specific mail-moving operation
\*------------------------------------------------------------------------------*/
class BmMailMoverView : public BmJobStatusView {
	typedef BmJobStatusView inherited;

public:
	// creator-func, c'tors and d'tor:
	static BmMailMoverView* CreateInstance( const char* name);
	BmMailMoverView( const char* name);
	~BmMailMoverView();

	// overrides of jobstatusview base:
	bool AlwaysRemoveWhenDone()			{ return true; }
	void ResetController();
	void UpdateModelView( BMessage* msg);
	BmJobModel* CreateJobModel( BMessage* msg);

	// overrides of controller base:
	BHandler* GetControllerHandler() 	{ return this; }

private:
	BStatusBar* mStatBar;
							// shows number of mails moved during this operation
	MStringView* mBottomLabel;

	// Hide copy-constructor and assignment:
	BmMailMoverView( const BmMailMoverView&);
	BmMailMoverView operator=( const BmMailMoverView&);
};

/*------------------------------------------------------------------------------*\
	BmMailFilterView
		-	controls a specific mail-filtering operation
\*------------------------------------------------------------------------------*/
class BmMailFilterView : public BmJobStatusView {
	typedef BmJobStatusView inherited;

public:
	// creator-func, c'tors and d'tor:
	static BmMailFilterView* CreateInstance( const char* name);
	BmMailFilterView( const char* name);
	~BmMailFilterView();

	// overrides of jobstatusview base:
	bool AlwaysRemoveWhenDone()			{ return true; }
	void ResetController();
	void UpdateModelView( BMessage* msg);
	BmJobModel* CreateJobModel( BMessage* msg);

	// overrides of controller base:
	BHandler* GetControllerHandler() 	{ return this; }

private:
	BStatusBar* mStatBar;
							// shows number of mails filtered during this operation
	MStringView* mBottomLabel;

	// Hide copy-constructor and assignment:
	BmMailFilterView( const BmMailFilterView&);
	BmMailFilterView operator=( const BmMailFilterView&);
};

/*------------------------------------------------------------------------------*\
	BmPopperView
		-	controls a specific POP3-connection
\*------------------------------------------------------------------------------*/
class BmPopperView : public BmJobStatusView {
	typedef BmJobStatusView inherited;

public:
	// creator-func, c'tors and d'tor:
	static BmPopperView* CreateInstance( const char* name, bool isAutoCheck);
	BmPopperView( const char* name, bool isAutoCheck);
	~BmPopperView();

	// overrides of jobstatusview base:
	void ResetController();
	void UpdateModelView( BMessage* msg);
	BmJobModel* CreateJobModel( BMessage* msg);

	// overrides of controller base:
	BHandler* GetControllerHandler() 	{ return this; }

private:
	BStatusBar* mStatBar;
							// shows current status of this connection
	BStatusBar* mMailBar;
							// shows number of mails handled by this connection
	bool mHaveBeeped;	
							// have we indicated arrival of new mail?

	// Hide copy-constructor and assignment:
	BmPopperView( const BmPopperView&);
	BmPopperView operator=( const BmPopperView&);
};

/*------------------------------------------------------------------------------*\
	BmImapView
		-	controls a specific IMAP-connection
\*------------------------------------------------------------------------------*/
class BmImapView : public BmJobStatusView {
	typedef BmJobStatusView inherited;

public:
	// creator-func, c'tors and d'tor:
	static BmImapView* CreateInstance( const char* name, bool isAutoCheck);
	BmImapView( const char* name, bool isAutoCheck);
	~BmImapView();

	// overrides of jobstatusview base:
	void ResetController();
	void UpdateModelView( BMessage* msg);
	BmJobModel* CreateJobModel( BMessage* msg);

	// overrides of controller base:
	BHandler* GetControllerHandler() 	{ return this; }

private:
	BStatusBar* mStatBar;
							// shows current status of this connection
	BStatusBar* mMailBar;
							// shows number of mails handled by this connection
	bool mHaveBeeped;	
							// have we indicated arrival of new mail?

	// Hide copy-constructor and assignment:
	BmImapView( const BmImapView&);
	BmImapView operator=( const BmImapView&);
};

/*------------------------------------------------------------------------------*\
	BmSmtpView
		-	controls a specific SMTP-connection
\*------------------------------------------------------------------------------*/
class BmSmtpView : public BmJobStatusView {
	typedef BmJobStatusView inherited;

public:
	// creator-func, c'tors and d'tor:
	static BmSmtpView* CreateInstance( const char* name);
	BmSmtpView( const char* name);
	~BmSmtpView();

	// overrides of jobstatusview base:
	void ResetController();
	void UpdateModelView( BMessage* msg);
	BmJobModel* CreateJobModel( BMessage* msg);

	// overrides of controller base:
	BHandler* GetControllerHandler() 	{ return this; }

private:
	BStatusBar* mStatBar;
							// shows current status of this connection
	BStatusBar* mMailBar;
							// shows number of mails handled by this connection

	// Hide copy-constructor and assignment:
	BmSmtpView( const BmSmtpView&);
	BmSmtpView operator=( const BmSmtpView&);
};

/*------------------------------------------------------------------------------*\
	BmJobStatusWin
		-	implements the connection-window, where the states of all 
			active connections are displayed
		-	three different display-modes are available:
			* DYNAMIC:			JobStatuss are only displayed as long as they are 
									active. When no connection is active, the window 
									will close.
			* DYNAMIC_EMPTY:	Only POP-connections that actually received mail 
									stay visible after the connection has ended
			* STATIC:			All connections are visible, even after 
									connection-close
		-	in general each connection to any server starts as a request to this 
			class (better: the one and only instance of this class). 
			For every requested connection a new interface is generated and 
			displayed, then the connections is executed by the corresponding 
			connection-object (for instance a BmPopper).
		-	connections and their interfaces are connected via the interface 
			defined between BmJobController and BmJobModel.
\*------------------------------------------------------------------------------*/
class BmJobStatusWin : public BmWindow {
	typedef BmWindow inherited;

	friend class BmJobStatusView;

	typedef map<BmString, BmJobStatusView*> JobMap;
	typedef deque<BMessage*> JobQueue;

public:
	// creator-func, c'tors and d'tor:
	static BmJobStatusWin* CreateInstance();
	BmJobStatusWin();
	virtual ~BmJobStatusWin();

	// overrides of BWindow base:
	bool QuitRequested();
	void Quit();
	void MessageReceived( BMessage* msg);

	// getters:
	bool HasActiveJobs();

	//
	static const rgb_color BM_COL_STATUSBAR;
	static const rgb_color BM_COL_STATUSBAR_GOOD;
	static const rgb_color BM_COL_STATUSBAR_BAD;

	static BmJobStatusWin* theInstance;

private:

	// native methods:
	void AddJob( BMessage* msg);
	void RemoveJob( const char* name);
	void QueueJob( BMessage* msg);

	JobMap mActiveJobs;
							// running jobs
	JobMap mDoneJobs;
							// jobs that are done, waiting to be removed
	JobQueue mQueuedJobs;
							// jobs waiting to run
	VGroup* mOuterGroup;
							// the outmost view that the connection-interfaces live in
	BLooper* mInvokingLooper;
							// the looper we will tell that we are finished

	// Hide copy-constructor and assignment:
	BmJobStatusWin( const BmJobStatusWin&);
	BmJobStatusWin operator=( const BmJobStatusWin&);
};

#define TheJobStatusWin BmJobStatusWin::theInstance


#endif
