/*
	BmMailRef.cpp
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


#include <ctype.h>

#include <File.h>
#include <NodeMonitor.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailFolderList.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

static BmString BM_REFKEYSTAT( const struct stat& x) {
	return BmString() << x.st_ino;
}

	// archival-fieldnames:
const char* const BmMailRef::MSG_ACCOUNT = 	"bm:ac";
const char* const BmMailRef::MSG_ATTACHMENTS= "bm:at";
const char* const BmMailRef::MSG_CC = 			"bm:cc";
const char* const BmMailRef::MSG_ENTRYREF = 	"bm:er";
const char* const BmMailRef::MSG_FROM = 		"bm:fr";
const char* const BmMailRef::MSG_INODE = 		"bm:in";
const char* const BmMailRef::MSG_NAME = 		"bm:nm";
const char* const BmMailRef::MSG_PRIORITY = 	"bm:pr";
const char* const BmMailRef::MSG_WHEN_CREATED = "bm:cr";
const char* const BmMailRef::MSG_REPLYTO = 	"bm:rp";
const char* const BmMailRef::MSG_SIZE = 		"bm:sz";
const char* const BmMailRef::MSG_STATUS = 	"bm:st";
const char* const BmMailRef::MSG_SUBJECT = 	"bm:su";
const char* const BmMailRef::MSG_TO = 			"bm:to";
const char* const BmMailRef::MSG_WHEN = 		"bm:wh";
const char* const BmMailRef::MSG_IDENTITY = 	"bm:id";
const char* const BmMailRef::MSG_IS_VALID = 	"bm:iv";
const int16 BmMailRef::nArchiveVersion = 4;

/*------------------------------------------------------------------------------*\
	CreateInstance( )
		-	static creator-func
		-	N.B.: In here, we lock the GlobalLocker manually (*not* BmAutolock),
			because otherwise we may risk deadlocks
\*------------------------------------------------------------------------------*/
BmRef<BmMailRef> BmMailRef::CreateInstance( BmMailRefList* model, 
														  entry_ref &eref, 
												  		  struct stat& st) {
	GlobalLocker()->Lock();
	if (!GlobalLocker()->IsLocked()) {
		BM_SHOWERR("BmMailRef::CreateInstance(): Could not acquire global lock!");
		return NULL;
	}
	BmProxy* proxy = BmRefObj::GetProxy( typeid(BmMailRef).name());
	if (proxy) {
		BmString key( BM_REFKEYSTAT( st));
		BmRef<BmMailRef> mailRef( 
			dynamic_cast<BmMailRef*>( proxy->FetchObject( key))
		);
		GlobalLocker()->Unlock();
		if (mailRef) {
			// update mailref with new info:
			if (model) {
				// only update list if we really have one (don't clobber existing
				// list with NULL value):
				mailRef->mListModel = model;
			}
			mailRef->EntryRef( eref);
			mailRef->ResyncFromDisk();
			return mailRef;
		}
		mailRef = new BmMailRef( model, eref, st);
		mailRef->Initialize();
		return mailRef;
	}
	GlobalLocker()->Unlock();
	BM_SHOWERR( BmString("Could not get proxy for ") 
						<< typeid(BmMailRef).name());
	return NULL;
}

/*------------------------------------------------------------------------------*\
	CreateInstance( )
		-	static creator-func
		-	N.B.: In here, we lock the GlobalLocker manually (*not* BmAutolock),
			because otherwise we may risk deadlocks
\*------------------------------------------------------------------------------*/
BmRef<BmMailRef> BmMailRef::CreateInstance( BMessage* archive, 
														  BmMailRefList* model) {
	GlobalLocker()->Lock();
	if (!GlobalLocker()->IsLocked()) {
		BM_SHOWERR("BmMailRef::CreateInstance(): Could not acquire global lock!");
		return NULL;
	}
	BmProxy* proxy = BmRefObj::GetProxy( typeid(BmMailRef).name());
	if (proxy) {
		status_t err;
		node_ref nref;
		if ((err = archive->FindInt64( MSG_INODE, &nref.node)) != B_OK) {
			BM_LOGERR( BmString("BmMailRef: Could not find msg-field ") 
								<< MSG_INODE << "\n\nError:" << strerror(err));
			return NULL;
		}
		nref.device = ThePrefs->MailboxVolume.Device();
		BmString key( BM_REFKEY( nref));
		BmRef<BmMailRef> mailRef( 
			dynamic_cast<BmMailRef*>( proxy->FetchObject( key))
		);
		GlobalLocker()->Unlock();
		if (mailRef) {
			// update mailref with new info:
			if (model) {
				// only update list if we really have one (don't clobber existing
				// list with NULL value):
				mailRef->mListModel = model;
			}
			return mailRef;
		}
		mailRef = new BmMailRef( archive, nref, model);
		mailRef->Initialize();
		return mailRef;
	}
	GlobalLocker()->Unlock();
	BM_SHOWERR( BmString("Could not get proxy for ") 
						<< typeid(BmMailRef).name());
	return NULL;
}

/*------------------------------------------------------------------------------*\
	BmMailRef( eref, parent, modified)
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailRef::BmMailRef( BmMailRefList* model, entry_ref &eref, struct stat& st)
	:	inherited( BM_REFKEYSTAT(st), model, (BmListModelItem*)NULL)
	,	mEntryRef( eref)
	,	mInitCheck( B_NO_INIT)
{
	mNodeRef.device = st.st_dev;
	mNodeRef.node = st.st_ino;
}

/*------------------------------------------------------------------------------*\
	BmMailRef( archive)
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
BmMailRef::BmMailRef( BMessage* archive, node_ref& nref, BmMailRefList* model)
	:	inherited( "", model, (BmListModelItem*)NULL)
	,	mInitCheck( B_NO_INIT)
	,	mNodeRef( nref)
{
	try {
		status_t err;
		if ((err = archive->FindRef( MSG_ENTRYREF, &mEntryRef)) != B_OK)
			BM_THROW_RUNTIME( BmString("BmMailRef: Could not find msg-field ") 
										<< MSG_ENTRYREF << "\n\nError:" << strerror(err));
		mEntryRef.device = nref.device;
		Key( BM_REFKEY( mNodeRef));

		int16 version = 0;
		archive->FindInt16( MSG_VERSION, &version);

		mAccount = FindMsgString( archive, MSG_ACCOUNT);
		mHasAttachments = FindMsgBool( archive, MSG_ATTACHMENTS);
		mCc = FindMsgString( archive, MSG_CC);
		mFrom = FindMsgString( archive, MSG_FROM);
		mName = FindMsgString( archive, MSG_NAME);
		mPriority = FindMsgString( archive, MSG_PRIORITY);
		mReplyTo = FindMsgString( archive, MSG_REPLYTO);
		mSize = FindMsgInt64( archive, MSG_SIZE);
		mStatus = FindMsgString( archive, MSG_STATUS);
		mSubject = FindMsgString( archive, MSG_SUBJECT);
		mTo = FindMsgString( archive, MSG_TO);
		mWhen = FindMsgInt32( archive, MSG_WHEN);

		if (version >= 2)
			mIdentity = FindMsgString( archive, MSG_IDENTITY);

		if (version >= 3)
			mWhenCreated = FindMsgInt64( archive, MSG_WHEN_CREATED);
		else
			mWhenCreated = static_cast<int64>(
				FindMsgInt32( archive, MSG_WHEN_CREATED)
			)*(1000*1000);

		if (version >= 4)
			mItemIsValid = FindMsgBool( archive, MSG_IS_VALID);

		mSizeString = BytesToString( mSize,true);
		mWhenCreatedString = ThePrefs->GetBool( "UseSwatchTimeInRefView", false)
								? TimeToSwatchString( mWhenCreated/(1000*1000))
								: TimeToString( mWhenCreated/(1000*1000));
		mWhenString = 	ThePrefs->GetBool( "UseSwatchTimeInRefView", false)
								? TimeToSwatchString( mWhen)
								: TimeToString( mWhen);

		mInitCheck = B_OK;
	} catch (BM_error &e) {
		BM_SHOWERR( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	~BmMailRef()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmMailRef::~BmMailRef() {
	BM_LOG3( BM_LogMailTracking, 
				BmString("destructor of MailRef ") << Key() << " called");
	WatchNode( &mNodeRef, B_STOP_WATCHING, TheMailMonitor);
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailRef::Archive( BMessage* archive, bool) const {
	status_t ret 
		= archive->AddInt16( MSG_VERSION, nArchiveVersion)
		|| archive->AddBool( MSG_IS_VALID, mItemIsValid)
		|| archive->AddString( MSG_ACCOUNT, mAccount.String())
		|| archive->AddBool( MSG_ATTACHMENTS, mHasAttachments)
		|| archive->AddString( MSG_CC, mCc.String())
		|| archive->AddRef( MSG_ENTRYREF, &mEntryRef)
		|| archive->AddString( MSG_FROM, mFrom.String())
		|| archive->AddInt64( MSG_INODE, mNodeRef.node)
		|| archive->AddString( MSG_NAME, mName.String())
		|| archive->AddString( MSG_PRIORITY, mPriority.String())
		|| archive->AddInt64( MSG_WHEN_CREATED, mWhenCreated)
		|| archive->AddString( MSG_REPLYTO, mReplyTo.String())
		|| archive->AddInt64( MSG_SIZE, mSize)
		|| archive->AddString( MSG_STATUS, mStatus.String())
		|| archive->AddString( MSG_SUBJECT, mSubject.String())
		|| archive->AddString( MSG_TO, mTo.String())
		|| archive->AddString( MSG_IDENTITY, mIdentity.String())
		|| archive->AddInt32( MSG_WHEN, mWhen);
	return ret;
}

/*------------------------------------------------------------------------------*\
	Initialize()
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
void BmMailRef::Initialize() {
	WatchNode( &mNodeRef, B_WATCH_STAT | B_WATCH_ATTR, TheMailMonitor);
	if (mInitCheck != B_OK) {
		if (ReadAttributes())
			mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	ReadAttributes()
		-	reads attribute-data from mail-file
\*------------------------------------------------------------------------------*/
bool BmMailRef::ReadAttributes( const struct stat* statInfo) {
	status_t err;
	BNode node;
	BmString filetype;
	struct stat st;
	
	if (statInfo)
		st = *statInfo;
	
	for( int i=0; (err = node.SetTo( &mEntryRef)) == B_BUSY; ++i) {
		if (i==200)
			break;
		BM_LOG2( BM_LogMailTracking, 
					BmString("Node is locked for mail-ref <") << mEntryRef.name 
						<< ">. We take a nap and try again...");
		snooze( 10*1000);					// pause for 10ms
	}
	if (err != B_OK) {
		BM_LOG2(
			BM_LogMailTracking, 
			BmString("Could not get node for mail-ref <") 
				<< mEntryRef.name << "> \n\nError:" << strerror(err)
		);
	}
	if (err == B_OK && !statInfo) {
		if ((err = node.GetStat( &st)) != B_OK)
			BM_LOGERR(
				BmString("Could not get stat-info for mail-ref <") 
					<< mEntryRef.name << "> \n\nError:" << strerror(err)
			);
	}

	BmReadStringAttr( &node, "BEOS:TYPE", filetype);
	if (err == B_OK
	&& (!filetype.ICompare("text/x-email") 
		|| !filetype.ICompare("message/rfc822"))) {
		// file is indeed a mail, we fetch its attributes:
		BmReadStringAttr( &node, BM_MAIL_ATTR_NAME, 		mName);
		BmReadStringAttr( &node, BM_MAIL_ATTR_ACCOUNT, 	mAccount);
		BmReadStringAttr( &node, BM_MAIL_ATTR_CC, 		mCc);
		BmReadStringAttr( &node, BM_MAIL_ATTR_FROM, 		mFrom);
		BmReadStringAttr( &node, BM_MAIL_ATTR_PRIORITY, mPriority);
		BmReadStringAttr( &node, BM_MAIL_ATTR_REPLY, 	mReplyTo);
		BmReadStringAttr( &node, BM_MAIL_ATTR_STATUS, 	mStatus);
		BmReadStringAttr( &node, BM_MAIL_ATTR_SUBJECT, 	mSubject);
		BmReadStringAttr( &node, BM_MAIL_ATTR_TO, 		mTo);
		BmReadStringAttr( &node, BM_MAIL_ATTR_IDENTITY, mIdentity);

		mWhen = 0;
		node.ReadAttr( BM_MAIL_ATTR_WHEN, B_TIME_TYPE, 0, 
							&mWhen, sizeof(time_t));
		mWhenString = 	ThePrefs->GetBool( "UseSwatchTimeInRefView", false)
								? TimeToSwatchString( mWhen)
								: TimeToString( mWhen);

		int32 att1 = 0;
						// standard BeOS kind (BMail, Postmaster, Beam)
		node.ReadAttr( BM_MAIL_ATTR_ATTACHMENTS, B_INT32_TYPE, 0, 
							&att1, sizeof(att1));
		bool att2 = false;
						// Scooby kind
		node.ReadAttr( "MAIL:attachment", B_BOOL_TYPE, 0, &att2, sizeof(att2));
		mHasAttachments = att1>0 || att2;
						// please notice that we ignore Mail-It, since
						// it does not give any proper indication 
						// (other than its internal status-attribute,
						// which we really do not want to look at...)

		mSize = st.st_size;
		mSizeString = BytesToString( mSize,true);

		if (node.ReadAttr( BM_MAIL_ATTR_WHEN_CREATED, B_UINT64_TYPE, 0, 
								 &mWhenCreated, sizeof(bigtime_t)) < 0) {
			// corresponding attribute doesn't exist, we fetch it from the
			// file's creation time (which is just time_t instead of bigtime_t):
			mWhenCreated = static_cast<int64>(	st.st_crtime)*(1000*1000);
						// yes, crtime contains the creation-time, trust me!
		}
		mWhenCreatedString 
			= ThePrefs->GetBool( "UseSwatchTimeInRefView", false)
				? TimeToSwatchString( mWhenCreated/(1000*1000))
				: TimeToString( mWhenCreated/(1000*1000));

		// simplify priority:
		if (!mPriority.Length()) {
			mPriority = "3";				// normal priority
		} else {
			if (isdigit(mPriority[0]))
				mPriority.Truncate(1);
			else {
				if (mPriority.FindFirst("Highest") != B_ERROR)
					mPriority = "1";
				else if (mPriority.FindFirst("High") != B_ERROR)
					mPriority = "2";
				else if (mPriority.FindFirst("Lowest") != B_ERROR)
					mPriority = "5";
				else if (mPriority.FindFirst("Low") != B_ERROR)
					mPriority = "4";
				else
					mPriority = "3";
			}
		}
		ItemIsValid( true);
	} else {
		// item is no mail, we mark it as invalid:
		mName = mEntryRef.name;
		mAccount = "";
		mCc = "";
		mFrom = "";
		mPriority = "";
		mReplyTo = "";
		mStatus = "";
		mSubject = "";
		mTo = "";
		mIdentity = "";
		mWhen = 0;
		mWhenString = "";
		mWhenCreated = 0;
		mWhenCreatedString = "";
		mHasAttachments = false;
		mSize = 0;
		mSizeString = "";
		mPriority = "";

		BM_LOG2( BM_LogMailTracking, 
					BmString("file <") << mEntryRef.name 
						<< " is not a mail, invalidating it.");
		ItemIsValid( false);
	}
	return err == B_OK;
}

/*------------------------------------------------------------------------------*\
	ResyncFromDisk()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::ResyncFromDisk( entry_ref* newRef) {
	if (newRef)
		mEntryRef = *newRef;
	if (ReadAttributes())
		mInitCheck = B_OK;
	TellModelItemUpdated( UPD_ALL);
	BmRef<BmListModel> listModel( ListModel());
	BmMailRefList* refList = dynamic_cast< BmMailRefList*>( listModel.Get());
	if (refList)
		refList->MarkAsChanged();
}

/*------------------------------------------------------------------------------*\
	MarkAs()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::MarkAs( const char* status) {
	if (InitCheck() != B_OK)
		return;
	try {
		BNode mailNode;
		status_t err;
		mStatus = status;
		if ((err = mailNode.SetTo( &mEntryRef)) != B_OK)
			BM_THROW_RUNTIME( 
				BmString( "Could not create node for current mail-file.\n\n"
							 " Result: ") 
					<< strerror(err)
			);
		mailNode.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, 
								  status, strlen( status)+1);
		TellModelItemUpdated( UPD_STATUS);
		BmRef<BmListModel> listModel( ListModel());
		BmMailRefList* refList = dynamic_cast< BmMailRefList*>( listModel.Get());
		if (refList)
			refList->MarkAsChanged();
	} catch( BM_error &e) {
		BM_SHOWERR(e.what());
	}
}
