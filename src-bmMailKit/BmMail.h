/*
	BmMail.h
		$Id$
*/

#ifndef _BmMail_h
#define _BmMail_h

#include <map>

#include <String.h>

#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	mail_format_error
		-	exception to indicate an error in the format of a mail-message
\*------------------------------------------------------------------------------*/
class BM_mail_format_error : public BM_runtime_error {
public:
	BM_mail_format_error (const BString& what_arg): BM_runtime_error (what_arg.String()) { }
	BM_mail_format_error (const char* const what_arg): BM_runtime_error (what_arg) { }
};

/*------------------------------------------------------------------------------*\
	BmMail 
		-	represents a single mail-message in Beam
		-	contains functionality to read/write mails from/to files
		- 	implements all mail-specific text-handling like header-parsing, en-/decoding,
			en-/decrypting
\*------------------------------------------------------------------------------*/
class BmMail {

public:
	BmMail( );
	BmMail( BString &msgText, const BString &msgUID, const BString &account);
	virtual ~BmMail();

	// getters:
	const BString& AccountName() const	{ return mAccountName; }
	const BString& UID() const				{ return mUID; }

	const BString& BCC() 					{ return mHeaders["Bcc"]; }
	const BString& CC()						{ return mHeaders["Cc"]; }
	const BString& Date();
	const BString& From();
	const BString& ReplyTo()				{ return mHeaders["Reply-To"]; }
	const BString& ReturnPath()			{ return mHeaders["Return-Path"]; }
	const BString& Status();
	const BString& Subject()				{ return mHeaders["Subject"]; }
	const BString& To();

	// setters:
	void AccountName( const BString &s) { mAccountName = s; }
	void UID( const BString &s) 			{ mUID = s; }
	
	void BCC( const BString &s) 			{ mHeaders["Bcc"] = s; }
	void CC( const BString &s) 			{ mHeaders["Cc"] = s; }
	void Date( const BString &s) 			{ mHeaders["Date"] = s; }
	void From( const BString &s) 			{ mHeaders["From"] = s; }
	void Subject( const BString &s) 		{ mHeaders["Subject"] = s; }
	void To( const BString &s) 			{ mHeaders["To"] = s; }

	// file-related:
	bool Store( );
	void StoreAttributes( BFile& mailFile);

private:
	typedef map<BString, BString> HeaderMap;
	HeaderMap mHeaders;						// contains complete headers as fieldname/fieldbody-pairs
	BString mText;								// text of complete message
	BString mUID;								// unique-ID of this message
	BString mAccountName;					// name of POP-account this message came from (or goes to)
	BString mStatus;							// status of this mail (client-status that is, e.g. "Read" or "New")
	bool mHasAttachments;					// flag indicating the presence of attachments

	bool mHasChanged;							// flag indicating new or edited mail
	BEntry mParentEntry;						// filesystem-entry for mailfolder this mail currently lives in
	BEntry mMailEntry;						// filesystem-entry for this mail (N.B. the entry may
													// be set although the mail does not yet exist on disk)

	void ParseHeader( const BString &header);
	void Set( BString &msgText, const BString &msgUID, const BString &account);	
	BString CreateBasicFilename();
};

#endif
