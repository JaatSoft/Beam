/*
	BmSmtp.h

		$Id$
*/

#ifndef _BmSmtp_h
#define _BmSmtp_h

#include <memory>

#include <Message.h>
#include <NetEndpoint.h>

#include "BmDataModel.h"
#include "BmUtil.h"

class BmSmtpAccount;

#define BM_SMTP_NEEDS_PWD						'bmSp'

/*------------------------------------------------------------------------------*\
	BmSmtp
		-	implements the SMTP-client
		-	each instance represents a single connection to a specific SMTP-server
		-	in general, each BmSmtp is started as a thread which exits when the
			SMTP-session has ended
\*------------------------------------------------------------------------------*/
class BmSmtp : public BmJobModel {
	typedef BmJobModel inherited;
	
public:
	//	message component definitions for status-msgs:
	static const char* const MSG_SMTP = 		"bm:smtp";
	static const char* const MSG_DELTA = 		"bm:delta";
	static const char* const MSG_TRAILING = 	"bm:trailing";
	static const char* const MSG_LEADING = 	"bm:leading";

	// message component definitions for additional info:
	static const char* const MSG_PWD = 	"bm:pwd";

	// job-specifier for checking server capabilities:
	static const int32 BM_CHECK_CAPABILITIES_JOB = 1;

	BmSmtp( const BString& name, BmSmtpAccount* account);
	virtual ~BmSmtp();

	BString Name() const						{ return ModelName(); }

	bool StartJob();

private:
	static int32 FeedbackTimeout;			// the time a BmSmtp will allow to pass
													// before reacting on any user-action 
													// (like closing the window)

	static const int32 NetBufSize = 2048;

	BmRef<BmSmtpAccount> mSmtpAccount;	// Info about our smtp-account

	BNetEndpoint mSmtpServer;				// network-connection to SMTP-server
	bool mConnected;							// are we connected to the server?

	BString mAnswer;							// holds last answer of SMTP-server

	int32 mState;								// current SMTP-state (refer enum below)
	enum States {
		SMTP_AUTH_VIA_POP = 0,
		SMTP_CONNECT,
		SMTP_HELO,
		SMTP_AUTH,
		SMTP_SEND,
		SMTP_QUIT,
		SMTP_DONE,
		SMTP_FINAL
	};

	// stuff needed for internal SMTP-state-loop:
	typedef void (BmSmtp::*TStateMethod)();
	struct SmtpState {
		const char* text;
		TStateMethod func;
		SmtpState( const char* t, TStateMethod f) : text(t), func(f) { }
	};
	static SmtpState SmtpStates[SMTP_FINAL];

	// private functions:
	void Connect();
	void Helo();
	void AuthViaPopServer();
	void Auth();
	void SendMails();
	void Disconnect();
	void Quit( bool WaitForAnswer=false);
	void Mail( BmMail *mail);
	void Rcpt( BmMail *mail);
	void BccRcpt( BmMail *mail, bool sendDataForEachBcc);
	void Data( BmMail *mail, BString forBcc="");
	void UpdateSMTPStatus( const float, const char*, bool failed=false);
	void UpdateMailStatus( const float, const char*, int32);
	void StoreAnswer( char* );
	bool CheckForPositiveAnswer();
	bool GetAnswer();
	int32 ReceiveBlock( char* buffer, int32 max);
	void SendCommand( BString cmd, BString secret="");

	bool mServerMayHaveSizeLimit;
	bool mServerSupportsDSN;

	// Hide copy-constructor and assignment:
	BmSmtp( const BmSmtp&);
	BmSmtp operator=( const BmSmtp&);
};

#endif
