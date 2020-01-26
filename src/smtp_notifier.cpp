#include "monitor_job.h"

#include "Poco/Net/MailRecipient.h"
#include "Poco/Net/SMTPClientSession.h"
#include "Poco/Net/SecureSMTPClientSession.h"
#include "Poco/Net/InvalidCertificateHandler.h"
#include "Poco/Net/AcceptCertificateHandler.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/SecureStreamSocket.h"

using namespace std;
using namespace Poco::Net;

SmtpNotifier::SmtpNotifier(PTR_UMAP_STR params) {
  this->isSSL = false;
  this->host = string();
  this->port = 0;
  this->username = string();
  this->password = string();

  for( const auto& n : *params )
    if ("proto" == n.first) {
			if (n.second == "plain")
				this->isSSL = false;
      else if (n.second == "ssl")
				this->isSSL = true;
			else
				throw invalid_argument(fmt::format("Unrecognized smtp proto \"{}\".", 
					n.second));
    }
    else if ("host" == n.first)
      this->host = n.second;
    else if ("port" == n.first)
      this->port = stoi(n.second);
    else if ("username" == n.first)
      this->username = n.second;
    else if ("password" == n.first)
      this->password = n.second;
    else
      throw invalid_argument(fmt::format("Unrecognized smtp parameter \"{}\".", 
        n.first));

  if (this->port == 0) this->port = (this->isSSL) ? 465 : 25;

  // Check for missing params:
  if (this->host.empty())
    throw invalid_argument(fmt::format("Smtp host was unspecified, and is required."));
}

bool SmtpNotifier::Send(MailMessage *message) {
  if (this->isSSL) {
    Poco::SharedPtr<InvalidCertificateHandler> pCert = new AcceptCertificateHandler(false);
    Context::Ptr pContext = new Context(Context::CLIENT_USE, "", "", "", 
      Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

    SSLManager::instance().initializeClient(0, pCert, pContext);

    SecureStreamSocket pSSLSocket(pContext);
    pSSLSocket.connect(SocketAddress(this->host, this->port));
    SecureSMTPClientSession session(pSSLSocket);

    session.login();
		if (!this->username.empty() && !this->password.empty()) {
      // NOTE: The TLS May fail, or may succeed. It seems like there's no reason 
      // not to try if we're already SSL encrypted.
			session.startTLS(pContext);
			session.login(SMTPClientSession::AUTH_LOGIN, this->username, this->password);
		}

    session.sendMessage(*message);
    session.close();
  } else {
    // NOTE: This code path is untested. My ISP blocks outbound port 25, and I
    // don't really care to spend the time testing this.
		SMTPClientSession session(this->host, this->port); 

		session.login();
		if (!this->username.empty() && !this->password.empty()) {
			session.login(SMTPClientSession::AUTH_LOGIN, this->username, this->password);
		}
		session.sendMessage(*message);
		session.close();
  }

  return true;
}
