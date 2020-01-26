#include "monitor_job.h"

// TODO: Prune any of this?
#include "Poco/Net/MailRecipient.h"
#include "Poco/Net/SMTPClientSession.h"
#include "Poco/Net/NetException.h"
#include "Poco/Net/SecureSMTPClientSession.h"
#include "Poco/Net/InvalidCertificateHandler.h"
#include "Poco/Net/AcceptCertificateHandler.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/SecureStreamSocket.h"

using namespace std;
using namespace Poco::Net;

SmtpNotifier::SmtpNotifier(PTR_UMAP_STR params) {
  this->host = string();
  this->port = 25;
  this->username = string();
  this->password = string();

  for( const auto& n : *params )
    if ("host" == n.first)
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

  // TODO: check for missing params
}

bool SmtpNotifier::Send(MailMessage *message) {
  // TODO: Handle non-tls and non-auth

  Poco::SharedPtr<InvalidCertificateHandler> pCert = new AcceptCertificateHandler(false);
  Context::Ptr pContext = new Context(Context::CLIENT_USE, "", "", "", 
    Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

  SSLManager::instance().initializeClient(0, pCert, pContext);

  SecureStreamSocket pSSLSocket(pContext);
  pSSLSocket.connect(SocketAddress(this->host, this->port));
  SecureSMTPClientSession secure(pSSLSocket);

  secure.login();
  bool tlsStarted = secure.startTLS(pContext);
  secure.login(SMTPClientSession::AUTH_LOGIN, this->username, this->password);


  secure.sendMessage(*message);
  secure.close();

  return true;
}
