/*
	BmMail.cpp
		$Id$
*/

#include <ctime>

#include <NodeInfo.h>

#include <regexx/regexx.hh>
using namespace regexx;

#include "BmApp.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmPrefs.h"

#undef BM_LOGNAME
#define BM_LOGNAME mAccountName

/*------------------------------------------------------------------------------*\
	default constructor
\*------------------------------------------------------------------------------*/
BmMail::BmMail( ) 
	:	mHasChanged( false)
	,	mStatus( "New")
	,	mHasAttachments( false)
{
}

/*------------------------------------------------------------------------------*\
	constructor( msgText, msgUID)
		-	creates message with given text and unique-ID (as received from server)
\*------------------------------------------------------------------------------*/
BmMail::BmMail( BString &msgText, const BString &msgUID, const BString &account) 
	:	mHasChanged( true) 
	,	mHasAttachments( false)
{
	Set( msgText, msgUID, account);
	mStatus = "New";
}
	
/*------------------------------------------------------------------------------*\
	destructor
\*------------------------------------------------------------------------------*/
BmMail::~BmMail() {
}

/*------------------------------------------------------------------------------*\
	Set( msgText, msgUID)
		-	initializes mail-object with given data
		-	the mail-header is extracted from msgText and is parsed
		-	account is the name of the POP/IMAP-account this message was received from
\*------------------------------------------------------------------------------*/
void BmMail::Set( BString &msgText, const BString &msgUID, const BString &account) {
	int32 headerLen = msgText.FindFirst( "\r\n\r\n");
							// STD11: empty-line seperates header from body
	if (headerLen == B_ERROR) {
		throw BM_mail_format_error("BmMail: Could not determine borderline between header and text of message");
	}
	headerLen += 2;							// include cr/nl in header-string

	mText.Adopt( msgText);					// take over the msg-string
	AccountName( account);
	UID( msgUID);

	BString header;
	header.SetTo( mText, headerLen);
	ParseHeader( header);
}
	
/*------------------------------------------------------------------------------*\
	ParseHeader( header)
		-	parses mail-header and splits it into fieldname/fieldbody - pairs
\*------------------------------------------------------------------------------*/
void BmMail::ParseHeader( const BString &header) {
	Regexx rxHeaderFields, rxUnfold;
	int32 nm;

	// split header into separate header-fields:
	rxHeaderFields.expr( "^(\\S.+?\\r\\n(?:\\s.+?\\r\\n)*)(?=(\\Z|\\S))");
	rxHeaderFields.str( header.String());
	if (!(nm=rxHeaderFields.exec( Regexx::global | Regexx::newline))) {
		throw BM_mail_format_error( BString("Could not find any header-fields in this header: \n") << header);
	}
	vector<RegexxMatch>::const_iterator i;

	BM_LOG( BM_LogMailParse, "The mail-header");
	BM_LOG3( BM_LogMailParse, BString(header) << "\n------------------");
	BM_LOG( BM_LogMailParse, BString("contains ") << nm << " headerfields\n");

	for( i = rxHeaderFields.match.begin(); i != rxHeaderFields.match.end(); ++i) {

		// split each headerfield into field-name and field-body:
		BString headerField, fieldName, fieldBody;
		header.CopyInto( headerField, i->start(), i->Length());
		int32 pos = headerField.FindFirst( ':');
		if (pos == B_ERROR) { throw BM_mail_format_error(""); }
		fieldName.SetTo( headerField, pos);
		fieldName.RemoveSet( bmApp->WHITESPACE);
		fieldName.CapitalizeEachWord();	
							// capitalized fieldnames seem to be popular...
		headerField.CopyInto( fieldBody, pos+1, headerField.Length());

		// unfold the field-body and remove leading and trailing whitespace:
		fieldBody = rxUnfold.replace( fieldBody, "\\r\\n\\s*", " ", Regexx::newline | Regexx::global);
		fieldBody = rxUnfold.replace( fieldBody, "^\\s+", "", Regexx::global);
		fieldBody = rxUnfold.replace( fieldBody, "\\s+$", "", Regexx::global);

		// insert pair into header-map:
		mHeaders[fieldName] = fieldBody;

		BM_LOG2( BM_LogMailParse, fieldName << ": " << fieldBody << "\n------------------");
	}
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores mail-data and attributes inside a file
\*------------------------------------------------------------------------------*/
bool BmMail::Store() {
	BFile mailFile;
	BNodeInfo mailInfo;
	BPath path;
	status_t err;
	ssize_t res;
	BString basicFilename;
	BString inboxPath;

	try {
		if (mParentEntry.InitCheck() == B_NO_INIT) {
			(inboxPath = bmApp->Prefs->MailboxPath()) << "/mailbox";
			(err = mParentEntry.SetTo( inboxPath.String(), true)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create entry for mail-folder <") << inboxPath << ">\n\n Result: " << strerror(err));
		}
		basicFilename = CreateBasicFilename();
		int max=100;
		for( int i=0; i<max; i++) {
			mParentEntry.GetPath( &path);
			BString filename( BString(path.Path()) << "/" << basicFilename);
			if (i) 
				filename << " " << i;
			(err = mMailEntry.SetTo( filename.String())) == B_OK
												|| BM_THROW_RUNTIME( BString("Could not create entry for new mail-file <") << filename << ">\n\n Result: " << strerror(err));
			if (!mMailEntry.Exists())
				break;
		}
		if (mMailEntry.Exists()) {
			BM_THROW_RUNTIME( BString("Unable to create a unique filename for mail <") << basicFilename << ">, giving up after "<<max<<" tries.\n\n Result: " << strerror(err));
		}
		// we create the new mailfile...
		(err = mailFile.SetTo( &mMailEntry, B_WRITE_ONLY | B_CREATE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create mail-file\n\t<") << basicFilename << ">\n\n Result: " << strerror(err));
		// ...set the correct mime-type...
		(err = mailInfo.SetTo( &mailFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not set node-info for mail-file\n\t<") << basicFilename << ">\n\n Result: " << strerror(err));
		mailInfo.SetType( "text/x-email");
		// ...store all other attributes...
		StoreAttributes( mailFile);
		// ...and finally write the raw mail into the file:
		int32 len = mText.Length();
		if ((res = mailFile.Write( mText.String(), len)) < len) {
			if (res < 0) {
				BM_THROW_RUNTIME( BString("Unable to write into mailfile <") << basicFilename << ">\n\n Result: " << strerror(err));
			} else {
				BM_THROW_RUNTIME( BString("Could not write complete mail into file.\nWrote ") << res << " bytes instead of " << len);
			}
		}
		mailFile.Sync();
	} catch( exception &e) {
		BM_SHOWERR(e.what());
		return false;
	}

	return true;
}

/*------------------------------------------------------------------------------*\
	StoreAttributes()
		-	stores mail-attributes inside a file
\*------------------------------------------------------------------------------*/
void BmMail::StoreAttributes( BFile& mailFile) {
	//
	mailFile.WriteAttr( "MAIL:name", B_STRING_TYPE, 0, mHeaders["Name"].String(), mHeaders["Name"].Length()+1);
	mailFile.WriteAttr( "MAIL:status", B_STRING_TYPE, 0, mStatus.String(), mStatus.Length()+1);
	mailFile.WriteAttr( "MAIL:account", B_STRING_TYPE, 0, mHeaders["Account"].String(), mHeaders["Account"].Length()+1);
	mailFile.WriteAttr( "MAIL:reply", B_STRING_TYPE, 0, mHeaders["Reply-To"].String(), mHeaders["Reply-To"].Length()+1);
	mailFile.WriteAttr( "MAIL:from", B_STRING_TYPE, 0, mHeaders["From"].String(), mHeaders["From"].Length()+1);
	mailFile.WriteAttr( "MAIL:subject", B_STRING_TYPE, 0, mHeaders["Subject"].String(), mHeaders["Subject"].Length()+1);
	mailFile.WriteAttr( "MAIL:to", B_STRING_TYPE, 0, mHeaders["To"].String(), mHeaders["To"].Length()+1);
	mailFile.WriteAttr( "MAIL:cc", B_STRING_TYPE, 0, mHeaders["Cc"].String(), mHeaders["Cc"].Length()+1);
	//
	time_t t = time(NULL);
	mailFile.WriteAttr( "MAIL:when", B_TIME_TYPE, 0, &t, sizeof(t));
	//
	mailFile.WriteAttr( "MAIL:attachments", B_BOOL_TYPE, 0, &mHasAttachments, sizeof(mHasAttachments));
}

/*------------------------------------------------------------------------------*\
	CreateBasicFilename()
		-	
\*------------------------------------------------------------------------------*/
BString BmMail::CreateBasicFilename() {
	BString name = mHeaders["From"];
	char now[16];
	time_t t = time(NULL);
	strftime( now, 15, "%0Y%0m%0d%0H%0M%0S", localtime( &t));
	name << "_" << now;
	name.ReplaceAll( "/", "-");
							// we avoid slashes in filename
	return name;
}