/*
	BmMailHeader.cpp
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


#include <algorithm>
#include <ctime>
#include <parsedate.h>

#include <List.h>
#include <NodeInfo.h>

#include "regexx.hh"
using namespace regexx;

#include "BmApp.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeader.h"
#include "BmNetUtil.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmSmtpAccount.h"

#undef BM_LOGNAME
#define BM_LOGNAME "MailParser"

static BString BmAddressFieldNames = 
	"<Bcc><Resent-Bcc><Cc><Resent-Cc><From><Resent-From><Reply-To><Resent-Reply-To><Sender><Resent-Sender><To><Resent-To>";

static BString BmNoEncodingFieldNames = 
	"<Received><Message-ID><Resent-Message-ID><Date><Resent-Date>";

/********************************************************************************\
	BmAddress
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddress::BmAddress( BString fullText)
	:	mInitOK( false)
{
	Regexx rx;
	BString addrText, phraseText;

	// first we check whether the addresstext is separated into phrase and addrspec:
	if (rx.exec( fullText, "^\\s*(.*?)\\s*<\\s*(?:.+:)?([^:]+)\\s*>\\s*$")) {
		// it's a phrase followed by an address (which possibly contains a source-route).
		fullText.CopyInto( phraseText, rx.match[0].atom[0].start(), rx.match[0].atom[0].Length());
		fullText.CopyInto( addrText, rx.match[0].atom[1].start(), rx.match[0].atom[1].Length());
		// strip leading&trailing quotes:
		if (rx.exec( phraseText, "^[\"']+(.+?)[\"']+$")) {
			phraseText.CopyInto( mPhrase, rx.match[0].atom[0].start(), rx.match[0].atom[0].Length());
		} else {
			mPhrase = phraseText;
		}
	} else {
		// it's just a simple address (no <>).
		addrText = fullText;
	}
	if (!addrText.Length()) {
		// address-part is empty!? not ok.
		return;
	}
	// finally strip all leading/trailing whitespace from the address-part:
	if (rx.exec( addrText, "^\\s*(.+?)\\s*$"))
		addrText.CopyInto( mAddrSpec, rx.match[0].atom[0].start(), rx.match[0].atom[0].Length());
	else
		mAddrSpec = addrText;
	mInitOK = true;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddress::~BmAddress() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddress::operator BString() const {
	if (mPhrase.Length()) {
		// quote the phrase if it contains "dangerous" characters:
		if (mPhrase.FindFirst(',')!=B_ERROR
		|| mPhrase.FindFirst(':')!=B_ERROR
		|| mPhrase.FindFirst('<')!=B_ERROR	
		|| mPhrase.FindFirst('>')!=B_ERROR)
			return BString("\"") << mPhrase << "\" <" << mAddrSpec << ">";
		else
			return mPhrase + " <" + mAddrSpec + ">";
	} else
		return mAddrSpec;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmAddress::IsHandledByAccount( BmPopAccount* acc) const {
	if (!acc)
		return false;
	return acc->HandlesAddress( mAddrSpec);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmAddress::ConstructRawText( BString& header, int32 encoding, int32 fieldNameLength) const {
	BString convertedAddrSpec = ConvertUTF8ToHeaderPart( mAddrSpec, encoding, false, 
																		  true, fieldNameLength);
	if (mPhrase.Length()) {
		// quote the phrase if it contains "dangerous" characters:
		BString phrase;
		if (mPhrase.FindFirst(',')!=B_ERROR
		|| mPhrase.FindFirst(':')!=B_ERROR
		|| mPhrase.FindFirst('<')!=B_ERROR	
		|| mPhrase.FindFirst('>')!=B_ERROR)
			phrase = BString("\"") << mPhrase << "\"";
		else
			phrase = mPhrase;
		BString convertedPhrase = ConvertUTF8ToHeaderPart( phrase, encoding, true, 
																			true, fieldNameLength);
		if (convertedPhrase.Length()+convertedAddrSpec.Length()+3 > ThePrefs->GetInt( "MaxLineLen")) {
			header << convertedPhrase << "\r\n";
			header.Append( BM_SPACES, fieldNameLength+2);
			header << "<" << convertedAddrSpec << ">";
		} else
			header << convertedPhrase << " <" << convertedAddrSpec << ">";
	} else
		header << convertedAddrSpec;
}


/********************************************************************************\
	BmAddressList
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddressList::BmAddressList()
	:	mInitOK( false)
	,	mIsGroup( false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddressList::BmAddressList( BString strippedFieldVal)
	:	mIsGroup( false) 
{
	Set( strippedFieldVal);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddressList::~BmAddressList() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmAddressList::Set( BString strippedFieldVal) {
	mGroupName = "";
	mAddrList.clear();

	mInitOK = Add( strippedFieldVal);
	return mInitOK;
}
	
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmAddressList::Add( BString strippedFieldVal) {
	BString addrText;
	Regexx rx;
	bool res = true;

	if (rx.exec( strippedFieldVal, "^\\s*(.+?):(.+)?;\\s*$")) {
		// it's a group list:
		mIsGroup = true;
		strippedFieldVal.CopyInto( mGroupName, rx.match[0].atom[0].start(), rx.match[0].atom[0].Length());
		if (rx.match[0].atom.size() > 1)
			strippedFieldVal.CopyInto( addrText, rx.match[0].atom[1].start(), rx.match[0].atom[1].Length());
		if (!addrText.Length())
			return false;
	} else {
		// simple address (or list of addresses)
		mIsGroup = false;
		addrText = strippedFieldVal;
	}
	BmStringList addrList = SplitIntoAddresses( addrText);
	size_t num = addrList.size();
	for( size_t i=0; i<num; ++i) {
		BmAddress addr( addrList[i]);
		if (addr.InitOK())
			mAddrList.push_back( addr);
		else
			res = false;
	}
	if (!mInitOK) 
		mInitOK = res;
	return res;
}
	
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmAddressList::Remove( BString singleAddress) {
	BmAddress rmAddr( singleAddress);
	BmAddrList::iterator pos = find( mAddrList.begin(), mAddrList.end(), rmAddr);
	if (pos != mAddrList.end())
		mAddrList.erase( pos);
}
	
/*------------------------------------------------------------------------------*\
	SplitIntoAddresses()
		-	
\*------------------------------------------------------------------------------*/
BString BmAddressList::FindAddressMatchingAccount( BmPopAccount* acc) const {
	int32 count = mAddrList.size();
	for( int i=0; i<count; ++i) {
		if (mAddrList[i].IsHandledByAccount( acc))
			return mAddrList[i];
	}
	return "";
}

/*------------------------------------------------------------------------------*\
	SplitIntoAddresses()
		-	
\*------------------------------------------------------------------------------*/
BmStringList BmAddressList::SplitIntoAddresses( BString addrListText) {
	BmStringList addrList;
	BString currAddr;
	const char* pos = addrListText.String();
	const char* endPos;
	while( *pos) {
		if (*pos == '"') {
			BString quotedString;
			// quoted-string started, we remove the quotes and unquote quoted-pairs.
			for( endPos=pos+1; *endPos && (*endPos!='"' || *(endPos-1)=='\\'); ++endPos)
				;
			if (*endPos) {
				// found complete quoted-string.
				int32 numChars = 1+endPos-pos;
				quotedString.Append( pos+1, numChars-2);
				pos += numChars;
			} else {
				// it seems that there is no ending quote, we assume the remainder to be 
				// part of the quoted string:
				quotedString.Append( pos+1);
				pos = endPos;
			}
			// we deescape characters that are escaped by a backslash (quoted-pairs):
			currAddr.Append( quotedString.CharacterDeescape( '\\'));
		} else if (*pos == '<') {
			// route-address started, we copy it as a block in order to avoid problems
			// with possibly contained separator-chars (commas).
			for( endPos=pos+1; *endPos && (*endPos!='>'); ++endPos)
				;
			if (*endPos) {
				// found complete route-address.
				int32 numChars = 1+endPos-pos;
				currAddr.Append( pos, numChars);
				pos += numChars;
			} else {
				// it seems that there is no ending '>', we assume the remainder to be 
				// part of the route-address (and append the missing '>'):
				currAddr.Append( pos);
				currAddr.Append( ">");
				pos = endPos;
			}
		} else {
			// we copy characters until we find the start of a quoted string or the separator char:
			for(  endPos=pos+1; *endPos && *endPos!='"' && *endPos!=','; ++endPos)
				;
			int32 numChars = endPos-pos;
			currAddr.Append( pos, numChars);
			pos += numChars;
			if (*endPos == ',') {
				addrList.push_back( currAddr);
				currAddr = "";
				pos++;
			}
		}
	}
	if (currAddr.Length()) {
		addrList.push_back( currAddr);
	}
	return addrList;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddressList::operator BString() const {
	BString addrString;
	if (mIsGroup)
		addrString << mGroupName << ": ";
	BmAddrList::const_iterator pos;
	for( pos=mAddrList.begin(); pos!=mAddrList.end(); ++pos) {
		if (pos != mAddrList.begin())
			addrString << ", ";
		addrString << BString( *pos);
	}
	if (mIsGroup)
		addrString << ";";
	return addrString;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmAddressList::ConstructRawText( BString& header, int32 encoding, 
												  int32 fieldNameLength) const {
	BString fieldString;
	if (mIsGroup)
		fieldString << mGroupName << ": ";
	BmAddrList::const_iterator pos;
	for( pos=mAddrList.begin(); pos!=mAddrList.end(); ++pos) {
		BString converted;
		pos->ConstructRawText( converted, encoding, fieldNameLength);
		if (pos != mAddrList.begin()) {
			fieldString += ", ";
			if (fieldString.Length() + converted.Length() > ThePrefs->GetInt( "MaxLineLen")) {
				fieldString += "\r\n";
				fieldString.Append( BM_SPACES, fieldNameLength+2);
			}
		}
		fieldString += converted;
	}
	if (mIsGroup)
		fieldString += ";";
	header += fieldString;
}



/********************************************************************************\
	BmHeaderList
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	Set( fieldName, value)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList::Set( const BString fieldName, const BString value) {
	BmValueList& valueList = mHeaders[fieldName];
	valueList.clear();
	valueList.push_back( value);
}

/*------------------------------------------------------------------------------*\
	Add( fieldName, value)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList::Add( const BString fieldName, const BString value) {
	BmValueList& valueList = mHeaders[fieldName];
	valueList.push_back( value);
}

/*------------------------------------------------------------------------------*\
	Remove( fieldName)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList::Remove( const BString fieldName) {
	mHeaders.erase( fieldName);
}

/*------------------------------------------------------------------------------*\
	operator [] ( fieldName)
		-	returns first value found for given fieldName
\*------------------------------------------------------------------------------*/
const BString& BmMailHeader::BmHeaderList::operator [] (const BString fieldName) const {
	BmHeaderMap::const_iterator iter = mHeaders.find(fieldName);
	if (iter == mHeaders.end())
		return BM_DEFAULT_STRING;
	const BmValueList& valueList = iter->second;
	if (valueList.empty())
		return BM_DEFAULT_STRING;
	return valueList.front();
}



/********************************************************************************\
	BmMailHeader
\********************************************************************************/

int32 BmMailHeader::nCounter = 0;

/*------------------------------------------------------------------------------*\
	BmMailHeader( headerText)
		-	constructor
\*------------------------------------------------------------------------------*/
BmMailHeader::BmMailHeader( const BString &headerText, BmMail* mail)
	:	mHeaderString( headerText)
	,	mMail( mail)
	,	mKey( RefPrintHex())					// generate dummy identifier from our address
	,	mIsRedirect( false)
{
	ParseHeader( headerText);
}
	
/*------------------------------------------------------------------------------*\
	~BmMailHeader()
	-	standard d'tor
\*------------------------------------------------------------------------------*/
BmMailHeader::~BmMailHeader() {
}

/*------------------------------------------------------------------------------*\
	IsAddressField()
	-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::IsAddressField( const BString fieldName) {
	BString fname = BString("<") << fieldName << ">";
	return BmAddressFieldNames.IFindFirst( fname) != B_ERROR;
}

/*------------------------------------------------------------------------------*\
	IsEncodingOkForField()
	-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::IsEncodingOkForField( const BString fieldName) {
	BString fname = BString("<") << fieldName << ">";
	return BmNoEncodingFieldNames.IFindFirst( fname) == B_ERROR;
}

/*------------------------------------------------------------------------------*\
	IsFieldEmpty()
	-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::IsFieldEmpty( const BString fieldName) {
	return GetFieldVal( fieldName).Length() == 0;
}

/*------------------------------------------------------------------------------*\
	GetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
const BString& BmMailHeader::GetFieldVal( const BString fieldName) {
	return mHeaders[fieldName];
}

/*------------------------------------------------------------------------------*\
	GetStrippedFieldVal()
	-	
\*------------------------------------------------------------------------------*/
const BString BmMailHeader::GetStrippedFieldVal( const BString fieldName) {
	if (IsAddressField( fieldName))
		return mAddrMap[fieldName];
	else
		return mStrippedHeaders[fieldName];
}

/*------------------------------------------------------------------------------*\
	GetAddressList()
		-	
\*------------------------------------------------------------------------------*/
const BmAddressList BmMailHeader::GetAddressList( const BString fieldName) {
	IsAddressField( fieldName)				|| BM_THROW_RUNTIME( BString("BmMailHeader.GetAddressList(): Field is not an address-field."));
	return mAddrMap[fieldName];
}

/*------------------------------------------------------------------------------*\
	SetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::SetFieldVal( const BString fieldName, const BString value) {
	mHeaders.Set( fieldName, value);
	BString strippedVal = StripField( value);
	mStrippedHeaders.Set( fieldName, strippedVal);
	if (IsAddressField( fieldName)) {
		// field contains an address-spec, we parse the address as well:
		mAddrMap[fieldName].Set( strippedVal);
	}
}

/*------------------------------------------------------------------------------*\
	AddFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::AddFieldVal( const BString fieldName, const BString value) {
	mHeaders.Add( fieldName, value);
	BString strippedVal = StripField( value);
	mStrippedHeaders.Add( fieldName, strippedVal);
	if (IsAddressField( fieldName)) {
		// field contains an address-spec, we parse the address as well:
		mAddrMap[fieldName].Add( strippedVal);
	}
}

/*------------------------------------------------------------------------------*\
	RemoveField()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::RemoveField( const BString fieldName) {
	mHeaders.Remove( fieldName);
	mStrippedHeaders.Remove( fieldName);
	mAddrMap.erase( fieldName);
}

/*------------------------------------------------------------------------------*\
	RemoveAddrFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::RemoveAddrFieldVal( const BString fieldName, const BString value) {
	if (IsAddressField( fieldName))
		mAddrMap[fieldName].Remove( value);
}

/*------------------------------------------------------------------------------*\
	DetermineSender()
		-	
\*------------------------------------------------------------------------------*/
BString BmMailHeader::DetermineSender() {
	BmAddressList addrList;
	if (ThePrefs->GetBool( "UseResentFieldsInReply", false)) {
		addrList = mAddrMap[BM_FIELD_RESENT_SENDER];
		if (!addrList.InitOK()) {
			addrList = mAddrMap[BM_FIELD_RESENT_FROM];
			if (!addrList.InitOK()) {
				BM_SHOWERR("Unable to determine sender of mail!");
				return "";
			}
		}
	} else {
		addrList = mAddrMap[BM_FIELD_SENDER];
		if (!addrList.InitOK()) {
			addrList = mAddrMap[BM_FIELD_FROM];
			if (!addrList.InitOK()) {
				BM_SHOWERR("Unable to determine sender of mail!");
				return "";
			}
		}
	}
	if (addrList.IsGroup())
		return addrList.GroupName();
	return addrList.FirstAddress().AddrSpec();
}

/*------------------------------------------------------------------------------*\
	DetermineSender()
		-	
\*------------------------------------------------------------------------------*/
BString BmMailHeader::DetermineReceivingAddrFor( BmPopAccount* acc) {
	BString addr;
	if (ThePrefs->GetBool( "UseResentFieldsInReply", false)) {
		addr = mAddrMap[BM_FIELD_RESENT_TO].FindAddressMatchingAccount( acc);
		if (!addr.Length()) {
			addr = mAddrMap[BM_FIELD_RESENT_CC].FindAddressMatchingAccount( acc);
			if (!addr.Length()) {
				addr = mAddrMap[BM_FIELD_RESENT_BCC].FindAddressMatchingAccount( acc);
			}
		}
	} else {
		addr = mAddrMap[BM_FIELD_TO].FindAddressMatchingAccount( acc);
		if (!addr.Length()) {
			addr = mAddrMap[BM_FIELD_CC].FindAddressMatchingAccount( acc);
			if (!addr.Length()) {
				addr = mAddrMap[BM_FIELD_BCC].FindAddressMatchingAccount( acc);
			}
		}
	}
	return addr;
}

/*------------------------------------------------------------------------------*\
	ParseHeader( header)
		-	parses mail-header and splits it into fieldname/fieldbody - pairs
		-	we try to determine the mails' encoding by finding a charset given within
			the header.	Some sending mail-clients cut some corners and incorporate 
			non-ASCII characters	unencoded inside any header-fields (the subject comes 
			to mind) [thus relying on the transport system being 8bit-clean].
			In this case we hope that the header contains a content-type specification 
			as well with the charset that was actually used. We feed the charset found 
			into the default encoding that is being used for header-field conversion 
			(if any only if nothing else is specified in a header-field)
\*------------------------------------------------------------------------------*/
void BmMailHeader::ParseHeader( const BString &header) {
	Regexx rxHeaderFields, rxUnfold, rx;
	int32 nm;

	// set default encoding
	mDefaultEncoding = ThePrefs->GetInt("DefaultEncoding");
	// we look for a default charset for this mail:
	rx.expr( "^Content-Type:\\s*.+?;charset\\s*=[\\s\"]*([^\\s\"]+)");
	rx.str( header.String());
	if (rx.exec( Regexx::nocase | Regexx::newline)) {
		// extract encoding from the charset found:
		BString charset;
		header.CopyInto( charset, rx.match[0].atom[0].start(), rx.match[0].atom[0].Length());
		mDefaultEncoding = CharsetToEncoding( charset);
	}

	// count number of lines in header
	mNumLines = rx.exec( header, "\\n", Regexx::newline | Regexx::global);

	// split header into separate header-fields:
	rxHeaderFields.expr( "^(\\S.*?\\r\\n(?:\\s.*?\\r\\n)*)(?=(\\Z|\\S))");
	rxHeaderFields.str( header.String());
	if (!(nm=rxHeaderFields.exec( Regexx::global | Regexx::newline)) && mMail) {
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
		if (pos == B_ERROR) { 
			BM_SHOWERR(BString("Could not determine field-name of mail-header-part:\n") << headerField << "\n\nThis header-field will be ignored."); 
			continue;
		}
		fieldName.SetTo( headerField, pos);
		fieldName.RemoveSet( TheResources->WHITESPACE);
		fieldName.CapitalizeEachWord();
							// capitalized fieldnames seem to be popular...
		headerField.CopyInto( fieldBody, pos+1, headerField.Length());

		// unfold the field-body and remove leading and trailing whitespace:
		fieldBody = rxUnfold.replace( fieldBody, "\\r\\n\\s*", " ", Regexx::newline | Regexx::global);
		fieldBody = rxUnfold.replace( fieldBody, "^\\s+", "", Regexx::global);
		fieldBody = rxUnfold.replace( fieldBody, "\\s+$", "", Regexx::global);

		// insert pair into header-map:
		AddFieldVal( fieldName, ConvertHeaderPartToUTF8( fieldBody, mDefaultEncoding));

		BM_LOG2( BM_LogMailParse, fieldName << ": " << fieldBody);
	}

	if (mAddrMap[BM_FIELD_RESENT_FROM].InitOK() || mAddrMap[BM_FIELD_RESENT_SENDER].InitOK())
		IsRedirect( true);

	if (mMail) {
		// we construct the 'name' for this mail (will go into attribute MAIL:name)...
		if (mMail->Outbound()) {
			// for outbound mails we fetch the groupname or phrase of the first TO-address:
			BmAddressList toAddrList = mAddrMap[BM_FIELD_TO];
			if (toAddrList.IsGroup()) {
				mName = toAddrList.GroupName();
			} else {
				BmAddress toAddr = toAddrList.FirstAddress();
				if (toAddr.HasPhrase())
					mName = toAddr.Phrase();
				else
					mName = toAddr.AddrSpec();
			}
		} else {
			// for inbound mails we fetch the groupname or phrase of the first FROM-address:
			BmAddressList fromAddrList = mAddrMap[BM_FIELD_FROM];
			if (fromAddrList.IsGroup()) {
				mName = fromAddrList.GroupName();
			} else {
				BmAddress fromAddr = fromAddrList.FirstAddress();
				if (fromAddr.HasPhrase())
					mName = fromAddr.Phrase();
				else
					mName = fromAddr.AddrSpec();
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	StripField()
		-	
\*------------------------------------------------------------------------------*/
BString BmMailHeader::StripField( BString fieldValue, BString* commentBuffer) {
	BString stripped;
	const char* pos = fieldValue.String();
	const char* endPos;
	while( *pos) {
		if (*pos == '"') {
			// quoted-string started, we search its end:
			for( endPos=pos+1; *endPos && (*endPos!='"' || *(endPos-1)=='\\'); ++endPos)
				;
			if (*endPos) {
				// found complete quoted-string, we copy it:
				int32 numChars = 1+endPos-pos;
				stripped.Append( pos, numChars);
				pos += numChars;
			} else {
				// it seems that there is no ending quote, we assume the remainder to be 
				// part of the quoted string (and add the missing quote):
				stripped.Append( pos);
				stripped.Append( "\"");
				pos = endPos;
			}
		} else if (*pos == '(') {
			// comment started, we search its end:
			BString comment;
			int32 nestLevel=1;
			for( endPos=pos+1; *endPos && (*endPos!=')' || *(endPos-1)=='\\' || --nestLevel); ++endPos) {
				if (*endPos == '(' && *(endPos-1)!='\\') {
					// take a note that we found a nested comment:
					nestLevel++;
				}
			}
			if (*endPos) {
				// found complete comment, we skip it:
				int32 numChars = 1+endPos-pos;
				if (commentBuffer)
					commentBuffer->Append( pos, numChars);
				pos += numChars;
			} else {
				// it seems that there is no ending paranthesis, so we assume that all the
				// remains are part of this incomplete comment. We add the missing paranthesis
				// to the comment buffer:
				if (commentBuffer) {
					commentBuffer->Append( pos);
					commentBuffer->Append( ")");
				}
				pos = endPos;
			}
		} else if (*pos == ' ' || *pos == '\t') {
			// replace linear whitespace by a single space:
			for( endPos=pos+1;  *endPos=='\t' || *endPos==' ';  ++endPos)
				;
			stripped.Append( " ");
			pos += endPos-pos;
		} else {
			// we copy characters until we find the start of a quoted-string,
			// whitespace, or a comment:
			for(  endPos=pos+1; 
					*endPos && *endPos!='"' && *endPos!='(' && *endPos!='\t' && *endPos!=' ';
					++endPos)
				;
			int32 numChars = endPos-pos;
			stripped.Append( pos, numChars);
			pos += numChars;
		}
	}
	return stripped;
}

/*------------------------------------------------------------------------------*\
	ParseDateTime()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::ParseDateTime( const BString& str, time_t& dateTime) {
	if (!str.Length()) return false;
	dateTime = parsedate( str.String(), -1);
	return dateTime != -1;
}

/*------------------------------------------------------------------------------*\
	StoreAttributes()
		-	stores mail-attributes inside a file
\*------------------------------------------------------------------------------*/
void BmMailHeader::StoreAttributes( BFile& mailFile) {
	//
	BString s = Name();
	mailFile.WriteAttr( BM_MAIL_ATTR_NAME, B_STRING_TYPE, 0, s.String(), s.Length()+1);
	s = mAddrMap[BM_FIELD_REPLY_TO];
	mailFile.WriteAttr( BM_MAIL_ATTR_REPLY, B_STRING_TYPE, 0, s.String(), s.Length()+1);
	s = mAddrMap[BM_FIELD_RESENT_FROM];
	if (!s.Length())
		s = mAddrMap[BM_FIELD_FROM];
	mailFile.WriteAttr( BM_MAIL_ATTR_FROM, B_STRING_TYPE, 0, s.String(), s.Length()+1);
	mailFile.WriteAttr( BM_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, 
							  mHeaders[BM_FIELD_SUBJECT].String(), mHeaders[BM_FIELD_SUBJECT].Length()+1);
	mailFile.WriteAttr( BM_MAIL_ATTR_MIME, B_STRING_TYPE, 0, 
							  mHeaders[BM_FIELD_MIME].String(), mHeaders[BM_FIELD_MIME].Length()+1);
	s = mAddrMap[BM_FIELD_RESENT_TO];
	if (!s.Length())
		s = mAddrMap[BM_FIELD_TO];
	mailFile.WriteAttr( BM_MAIL_ATTR_TO, B_STRING_TYPE, 0, s.String(), s.Length()+1);
	s = mAddrMap[BM_FIELD_RESENT_CC];
	if (!s.Length())
		s = mAddrMap[BM_FIELD_CC];
	mailFile.WriteAttr( BM_MAIL_ATTR_CC, B_STRING_TYPE, 0, s.String(), s.Length()+1);
	// we determine the mail's priority, first we look at X-Priority...
	BString priority = mHeaders[BM_FIELD_X_PRIORITY];
	// ...if that is not defined we check the Priority field:
	if (!priority.Length()) {
		// need to translate from text to number:
		BString prio = mHeaders[BM_FIELD_PRIORITY];
		if (!prio.ICompare("Highest")) priority = "1";
		else if (!prio.ICompare("High")) priority = "2";
		else if (!prio.ICompare("Normal")) priority = "3";
		else if (!prio.ICompare("Low")) priority = "4";
		else if (!prio.ICompare("Lowest")) priority = "5";
	}
	if (!priority.Length()) {
		priority = "3";						// we default to normal priority
	}
	mailFile.WriteAttr( BM_MAIL_ATTR_PRIORITY, B_STRING_TYPE, 0, priority.String(), priority.Length()+1);
	// if the message was resent, we take the date of the resending operation, not the
	// original date (note, however, that from- and reply-attributes and the like are 
	// filled from the original fields [the ones without "resent-"]).
	time_t t;
	if (!ParseDateTime( mHeaders[BM_FIELD_RESENT_DATE], t)
	&& !ParseDateTime( mHeaders[BM_FIELD_DATE], t))
		time( &t);
	mailFile.WriteAttr( BM_MAIL_ATTR_WHEN, B_TIME_TYPE, 0, &t, sizeof(t));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::ConstructRawText( BString& header, int32 encoding) {
	if (!mAddrMap[BM_FIELD_TO].InitOK() && !mAddrMap[BM_FIELD_CC].InitOK()) {
		if (mAddrMap[BM_FIELD_BCC].InitOK()) {
			// only hidden recipients via use of bcc, we set a dummy-<TO> value:
			SetFieldVal( BM_FIELD_TO, "Undisclosed-Recipients:;");
		}
	}

	mDefaultEncoding = encoding;

	// identify ourselves as creator of this mail message (so people know who to blame >:o)
	BString agentField = ThePrefs->GetBool( "PreferUserAgentOverX-Mailer", true)
								? BM_FIELD_USER_AGENT : BM_FIELD_X_MAILER;
	if (IsFieldEmpty( agentField)) {
		BString ourID = bmApp->BmAppNameWithVersion;
		SetFieldVal( agentField, ourID.String());
	}

	if (ThePrefs->GetBool( "GenerateOwnMessageIDs")) {
		// generate our own message-id:
		BString domain;
		BString accName = mMail->AccountName();
		if (accName.Length()) {
			BmSmtpAccount* account = dynamic_cast<BmSmtpAccount*>( TheSmtpAccountList->FindItemByKey( accName).Get());
			if (account)
				domain = account->DomainToAnnounce();
		}
		if (!domain.Length()) {
			// no account given or it has an empty domain, we try to find out manually:
			domain = OwnFQDN();
			if (!domain.Length()) {
				BM_SHOWERR( "Identity crisis!\nBeam is unable to determine full-qualified domainname of this computer, something is seriously wrong with network settings!\nBeam will use a fake name and continue");
				domain = "bepc.fake.local";
			}
		}
		SetFieldVal( mMail->IsRedirect() ? BM_FIELD_RESENT_MESSAGE_ID : BM_FIELD_MESSAGE_ID, 
						 BString("<") << TimeToString( time( NULL), "%Y%m%d%H%M%S.")
						 				  << find_thread(NULL) << "." << ++nCounter << "@" 
						 				  << domain << ">");
	}

	BmHeaderMap::const_iterator iter;
	if (mMail->IsRedirect()) {
		// add Resent-fields first (as suggested by [Johnson, section 2.4.2]):
		for( iter = mStrippedHeaders.begin(); iter != mStrippedHeaders.end(); ++iter) {
			const BString fieldName = iter->first;
			if (fieldName.ICompare("Resent-",7) != 0) {
				// just interested in Resent-fields:
				continue;
			}
			if (IsAddressField( fieldName)) {
				header << fieldName << ": ";
				mAddrMap[fieldName].ConstructRawText( header, encoding, fieldName.Length());
				header << "\r\n";
			} else {
				const BmValueList& valueList = iter->second;
				int count = valueList.size();
				bool encodeIfNeeded = IsEncodingOkForField( fieldName);
				for( int i=0; i<count; ++i) {
					header << fieldName << ": " 
							 << ConvertUTF8ToHeaderPart( valueList[i], encoding, encodeIfNeeded, 
																  true, fieldName.Length())
							 << "\r\n";
				}
			}
		}
	}
	// add all other fields:
	for( iter = mStrippedHeaders.begin(); iter != mStrippedHeaders.end(); ++iter) {
		const BString fieldName = iter->first;
		if (fieldName.ICompare("Content-",8) == 0) {
			// do not include MIME-header, since that will be added by body-part:
			continue;
		}
		if (fieldName.ICompare("Resent-",7) == 0) {
			// do not include Resent-headers again:
			continue;
		}
		if (IsAddressField( fieldName)) {
			header << fieldName << ": ";
			mAddrMap[fieldName].ConstructRawText( header, encoding, fieldName.Length());
			header << "\r\n";
		} else {
			const BmValueList& valueList = iter->second;
			int count = valueList.size();
			bool encodeIfNeeded = IsEncodingOkForField( fieldName);
			for( int i=0; i<count; ++i) {
				header << fieldName << ": " 
						 << ConvertUTF8ToHeaderPart( valueList[i], encoding, encodeIfNeeded, 
															  true, fieldName.Length())
						 << "\r\n";
			}
		}
	}
	return true;
}

