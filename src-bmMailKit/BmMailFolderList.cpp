/*
	BmMailFolderList.cpp
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


#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <Messenger.h>
#include <NodeInfo.h>
#include <Path.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmMailMonitor.h"
#include "BmMailRef.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/********************************************************************************\
	BmMailFolderList
\********************************************************************************/

BmRef< BmMailFolderList> BmMailFolderList::theInstance( NULL);

const char* const BmMailFolderList::MSG_MAILBOXMTIME = "bm:mboxmtime";
const int16 BmMailFolderList::nArchiveVersion = 2;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creator-func
\*------------------------------------------------------------------------------*/
BmMailFolderList* BmMailFolderList::CreateInstance() {
	if (!theInstance)
		theInstance = new BmMailFolderList();
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmMailFolderList()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailFolderList::BmMailFolderList()
	:	BmListModel( "MailFolderList", BM_LogMailTracking)
	,	mMailboxPathHasChanged( false)
{
}

/*------------------------------------------------------------------------------*\
	~BmMailFolderList()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmMailFolderList::~BmMailFolderList() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	AddSpecialFlag()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolder* BmMailFolderList::AddSpecialFlag( const node_ref& pnref, 
																const node_ref& nref) {
#ifdef BM_REF_DEBUGGING
	BM_ASSERT( ModelLocker().IsLocked());
#endif
	BmRef<BmListModelItem> parentRef = FindItemByKey( BM_REFKEY( pnref));
	BmMailFolder* parent = dynamic_cast< BmMailFolder*>( parentRef.Get());
	if (parent)
		parent->AddSpecialFlagForMailRef(BM_REFKEY(nref));
	return parent;
}

/*------------------------------------------------------------------------------*\
	RemoveSpecialFlag()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::RemoveSpecialFlag( const node_ref& pnref, 
												  		const node_ref& nref) {
#ifdef BM_REF_DEBUGGING
	BM_ASSERT( ModelLocker().IsLocked());
#endif
	BmRef<BmListModelItem> parentRef = FindItemByKey( BM_REFKEY( pnref));
	BmMailFolder* parent = dynamic_cast< BmMailFolder*>( parentRef.Get());
	if (parent)
		parent->RemoveSpecialFlagForMailRef(BM_REFKEY(nref));
}

/*------------------------------------------------------------------------------*\
	AddMailFolder()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolder* 
BmMailFolderList::AddMailFolder( entry_ref& eref, int64 node, 
										   BmMailFolder* parent, 
											time_t mtime) {
	BmMailFolder* newFolder = new BmMailFolder( this, eref, node, parent, mtime);
	AddItemToList( newFolder, parent);
	return newFolder;
}

/*------------------------------------------------------------------------------*\
	FindMailRefByKey()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmMailRef> BmMailFolderList::FindMailRefByKey( const node_ref& nref) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":FindMailRefByKey(): Unable to get lock"
		);
	BmRef<BmMailRef> ref;
	if (mTopFolder) {	
		BmString key = BM_REFKEY( nref);
		BmRef<BmListModelItem> item = mTopFolder->FindMailRefByKey( key);
		ref = dynamic_cast< BmMailRef*>( item.Get());
	}
	return ref;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolderList::StartJob() {
	if (inherited::StartJob()) {
		if (!mSpecialMailQuery.IsLive())
			QueryForSpecialMails();
		return true;
	} else
		return false;
}

/*------------------------------------------------------------------------------*\
	InitializeItems()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::InitializeItems() {
	BDirectory mailDir;
	BEntry entry;
	entry_ref eref;
	node_ref nref;
	status_t err;
	BmString mailDirName( ThePrefs->GetString("MailboxPath"));
	time_t mtime;

	BM_LOG( BM_LogMailTracking, "Start of initFolders");

	mailDir.SetTo( mailDirName.String());
	if ((err = mailDir.GetModificationTime( &mtime)) != B_OK)
		BM_THROW_RUNTIME( BmString("Could not get mtime \nfor mailbox-dir <") 
									<< mailDirName << "> \n\nError:" << strerror(err));
	if ((err = mailDir.GetEntry( &entry)) != B_OK)
		BM_THROW_RUNTIME( BmString("Could not get entry \nfor mailbox-dir <") 
									<< mailDirName << "> \n\nError:" << strerror(err));
	if ((err = entry.GetRef( &eref)) != B_OK)
		BM_THROW_RUNTIME( BmString("Could not get entry-ref \nfor mailbox-dir <") 
									<< mailDirName << "> \n\nError:" << strerror(err));
	if ((err = mailDir.GetNodeRef( &nref)) != B_OK)
		BM_THROW_RUNTIME( BmString("Could not get node-ref \nfor mailbox-dir <") 
									<< mailDirName << "> \n\nError:" << strerror(err));

	{	// scope for autolocker
		BM_LOG3( BM_LogMailTracking, 
					BmString("Top-folder <") << eref.name << "," << nref.node 
						<< "> found");
		mTopFolder = AddMailFolder( eref, nref.node, NULL, mtime);

		// now we process all subfolders of the top-folder recursively:
		int folderCount = 1 + doInitializeMailFolders( mTopFolder.Get(), 1);
		BM_LOG( BM_LogMailTracking, 
				  BmString("End of initFolders (") << folderCount 
				  		<< " folders found)");
		if (ShouldContinue())
			mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	doInitializeMailFolders()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFolderList::doInitializeMailFolders( BmMailFolder* folder, 
															  int level) {
	BDirectory mailDir;
	entry_ref eref;
	dirent* dent;
	struct stat st;
	status_t err;
	char buf[4096];
	int32 count, folderCount = 0;
	BmString mailDirName;

	// we create a BDirectory from the given mail-folder...
	mailDirName = folder->Name();
	mailDir.SetTo( folder->EntryRefPtr());
	if ((err = mailDir.InitCheck()) != B_OK)
		BM_THROW_RUNTIME( 
			BmString("Could not access \nmail-dir <") << mailDirName 
				<< "> \n\nError:" << strerror(err)
		);

	// ...and scan through all its entries for other mail-folders:
	while ((count = mailDir.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (count-- > 0) {
			if (!ShouldContinue())
				goto out;
			if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
				continue;						// ignore . and .. dirs
			if ((err = mailDir.GetStatFor( dent->d_name, &st)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not get stat-info for \nmail-dir <") 
						<< dent->d_name << "> \n\nError:" << strerror(err)
				);
			if (S_ISDIR( st.st_mode)) {
				// we have found a new mail-folder, so we add it as a child
				// of the current folder:
				eref.device = dent->d_pdev;
				eref.directory = dent->d_pino;
				eref.set_name( dent->d_name);
				BM_LOG3( BM_LogMailTracking, 
							BmString("Mail-folder <") << dent->d_name << "," 
								<< dent->d_ino << "> found at level " << level);
				BmMailFolder* newFolder = AddMailFolder( 
					eref, dent->d_ino, folder, st.st_mtime
				);
				folderCount++;
				// now we process the new sub-folder first:
				folderCount += doInitializeMailFolders( newFolder, level+1);
			}
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}
out:
	return folderCount;
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	(re-)creates top mail-folder from given archive and proceeds with all
			subfolders recursively
\*------------------------------------------------------------------------------*/
void BmMailFolderList::InstantiateItems( BMessage* archive) {
	status_t err;
	BM_LOG( BM_LogMailTracking, BmString("Starting to read folder-cache"));
	BMessage msg;
	if ((err = archive->FindMessage( 
		BmListModelItem::MSG_CHILDREN, &msg
	)) != B_OK)
		BM_THROW_RUNTIME( BmString("BmMailFolderList: Could not find msg-field <") 
									<< BmListModelItem::MSG_CHILDREN 
									<< "> \n\nError:" << strerror(err));
	{	// scope for autolock
		int32 folderCount = 1;
		mTopFolder = new BmMailFolder( &msg, this, NULL);
		BEntry entry( ThePrefs->GetString( "MailboxPath").String(), true);
		BNode node( &entry);
		node_ref nref;
		node.GetNodeRef( &nref);
		if (mTopFolder->NodeRef().node != nref.node) {
			// mailbox-folder has changed since last session, we start afresh:
			InitializeItems();
			return;
		}		
		AddItemToList( mTopFolder.Get());
		BM_LOG3( BM_LogMailTracking, 
					BmString("Top-folder <") << mTopFolder->EntryRef().name << "," 
						<< mTopFolder->Key() << "> read");
		if (mTopFolder->CheckIfModifiedSinceLastTime()) {
			folderCount += doInitializeMailFolders( mTopFolder.Get(), 1);
		} else {
			folderCount += doInstantiateMailFolders( mTopFolder.Get(), &msg, 1);
		}
		BM_LOG( BM_LogMailTracking, 
				  BmString("End of reading folder-cache (") << folderCount 
				  		<< " folders found)");
		mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	doInstantiateMailFolders( archive)
		-	recursively (re-)creates all mail-folders from given archive
\*------------------------------------------------------------------------------*/
int BmMailFolderList::doInstantiateMailFolders( BmMailFolder* folder, 
																BMessage* archive,
																int level) {
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmMailFolder::MSG_NUMCHILDREN);
	int32 folderCount = numChildren;
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		if ((err = archive->FindMessage( 
			BmMailFolder::MSG_CHILDREN, i, &msg
		)) != B_OK)
			BM_THROW_RUNTIME( BmString("Could not find mailfolder-child nr. ") 
										<< i+1 << " \n\nError:" << strerror(err));
		BmMailFolder* newFolder = new BmMailFolder( &msg, this, folder);
		AddItemToList( newFolder, folder);
		BM_LOG3( BM_LogMailTracking, 
					BmString("Mail-folder <") << newFolder->EntryRef().name 
						<< "," << newFolder->Key() << "> read");
		if (newFolder->CheckIfModifiedSinceLastTime()) {
			folderCount += doInitializeMailFolders( newFolder, level+1);
		} else {
			folderCount += doInstantiateMailFolders( newFolder, &msg, level+1);
		}
	}
	return folderCount;
}

/*------------------------------------------------------------------------------*\
	QueryForSpecialMails()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::QueryForSpecialMails() {
	int32 count, specialCount=0;
	status_t err;
	dirent* dent;
	node_ref pnref;
	node_ref nref;
	char buf[4096];

	typedef set<BmMailFolder*> BmFolderSet;
	BmFolderSet foldersWithSpecialMail;

	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":QueryForSpecialMails(): Unable to get lock"
		);

	BM_LOG( BM_LogMailTracking, "Start of special-mail-query");
	if ((err = mSpecialMailQuery.SetVolume( &ThePrefs->MailboxVolume)) != B_OK)
		BM_THROW_RUNTIME( BmString("SetVolume(): ") << strerror(err));
	// the following predicate exposes a bug in the way multiple values of a
	// single attribute are handled: in live mode BeOS actually sends *two* 
	// query updates for each message that matches the criteria...
	// We circumvent the problem by using proper id-keying in the code that
	// handles new/pending mails.
	if ((err = mSpecialMailQuery.SetPredicate( "(MAIL:status = 'New') || (MAIL:status = 'Pending')")) != B_OK)
		BM_THROW_RUNTIME( BmString("SetPredicate(): ") << strerror(err));
	if ((err = mSpecialMailQuery.SetTarget( BMessenger( TheMailMonitor))) != B_OK)
		BM_THROW_RUNTIME( 
			BmString("QueryForSpecialMails(): could not set query target.\n\nError:") 
				<< strerror(err)
		);
	if ((err = mSpecialMailQuery.Fetch()) != B_OK)
		BM_THROW_RUNTIME( BmString("Fetch(): ") << strerror(err));
	Freeze();
	while ((count = mSpecialMailQuery.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (count-- > 0) {
			specialCount++;
			pnref.device = dent->d_pdev;
			pnref.node = dent->d_pino;
			nref.device = dent->d_dev;
			nref.node = dent->d_ino;
			
			foldersWithSpecialMail.insert( AddSpecialFlag( pnref, nref));
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}
	Thaw();
	BmFolderSet::const_iterator iter;
	BmFolderSet::const_iterator end = foldersWithSpecialMail.end();
	for(	iter = foldersWithSpecialMail.begin(); iter != end; ++iter) {
		if (*iter)
			TellModelItemUpdated( 
				*iter, 
				BmMailFolder::UPD_SPECIAL_COUNT 
				| BmMailFolder::UPD_HAVE_SPECIAL_STATUS
			);
	}
	BM_LOG( BM_LogMailTracking, 
			  BmString("End of special-mail-query (") << specialCount 
			  		<< " special mails found)");
}

/*------------------------------------------------------------------------------*\
	RemoveController()
		-	stores the current state inside cache-file
\*------------------------------------------------------------------------------*/
void BmMailFolderList::RemoveController( BmController* controller) {
	inherited::RemoveController( controller);
	Store();
	if (mMailboxPathHasChanged) {
		// the user has selected a new mailbox, we remove all cache-files:
		BEntry folderCache( SettingsFileName().String());
		folderCache.Remove();
		// remove mailref-caches:
		BDirectory* mailCacheDir = BeamRoster->MailCacheFolder();
		mailCacheDir->Rewind();
		BEntry mailCache;
		while( mailCacheDir->GetNextEntry( &mailCache) == B_OK) {
			mailCache.Remove();
		}
		// remove state-info caches for mailref- & folder-listview:
		BDirectory* stateCacheDir = BeamRoster->StateInfoFolder();
		stateCacheDir->Rewind();
		BEntry stateCache;
		while( stateCacheDir->GetNextEntry( &stateCache) == B_OK) {
			char name[B_FILE_NAME_LENGTH+1];
			if (stateCache.GetName( name) == B_OK) {
				if (strncmp( name, "MailFolderView_", 15)==0 
				|| strncmp( name, "MailRefView_", 12)==0)
					stateCache.Remove();
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	
\*------------------------------------------------------------------------------*/
const BmString BmMailFolderList::SettingsFileName() {
	return BmString( BeamRoster->SettingsPath()) << "/" << "Folder Cache";
}
