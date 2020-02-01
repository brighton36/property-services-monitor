#include "monitor_job.h"

#include "Poco/Net/MailRecipient.h"
#include "Poco/Net/SMTPClientSession.h"
#include "Poco/Net/SecureSMTPClientSession.h"
#include "Poco/Net/InvalidCertificateHandler.h"
#include "Poco/Net/AcceptCertificateHandler.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/SecureStreamSocket.h"

#include <Poco/Net/StringPartSource.h>
#include <Poco/Net/FilePartSource.h>
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/MediaType.h>

#define CANT_READ "Unable to open file {}."

using namespace std;
using namespace Poco::Net;

SmtpNotifier::SmtpNotifier(string tpath, const YAML::Node config) {

  if (!config["to"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "to"));
  if (!config["from"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "from"));
  if (!config["subject"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "subject"));
  if (!config["template_html"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "template_html"));
  if (!config["template_plain"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "template_plain"));
  if (!config["host"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "host"));

  base_path = tpath;
  to = config["to"].as<string>();
  from = config["from"].as<string>();
  subject = config["subject"].as<string>();
  template_html_path = config["template_html"].as<string>();
  template_plain_path = config["template_plain"].as<string>();

  if (filesystem::path(template_html_path).is_relative())
    template_html_path = fmt::format("{}/{}", base_path, template_html_path);

  if (filesystem::path(template_plain_path).is_relative())
    template_plain_path = fmt::format("{}/{}", base_path, template_plain_path);

  if (!PathIsReadable(template_html_path)) 
    throw invalid_argument(fmt::format(CANT_READ, template_html_path));
  if (!PathIsReadable(template_plain_path)) 
    throw invalid_argument(fmt::format(CANT_READ, template_plain_path));

  host = config["host"].as<string>();
  isSSL = false;
  port = 0;
  username = string();
  password = string();

  if (config["port"]) port = stoi(config["port"].as<string>());
  if (config["username"]) username = config["username"].as<string>();
  if (config["password"]) password = config["password"].as<string>();
  if (config["proto"]) {
    auto proto = config["proto"].as<string>();

    if (proto == "plain") isSSL = false;
    else if (proto == "ssl") isSSL = true;
    else throw invalid_argument(fmt::format("Unrecognized smtp proto \"{}\".", proto));
  }
  
  if (port == 0) port = (isSSL) ? 465 : 25;

  // TODO: Do a template section like this:
	/*
	this->smtp_params = make_shared<unordered_map<string, string>>();
  
  if ( (!config["smtp"]) || (!config["smtp"].IsMap()))
    throw invalid_argument(fmt::format("Smtp settings missing", "smtp"));

  for(auto it=config["smtp"].begin();it!=config["smtp"].end();++it) {
    const auto param = it->first.as<std::string>();
    const auto value = it->second.as<std::string>();
    
    this->smtp_params->insert(make_pair(param, value));
  }*/
}

bool SmtpNotifier::DeliverMessage(MailMessage *message) {
  if (isSSL) {
    Poco::SharedPtr<InvalidCertificateHandler> pCert = new AcceptCertificateHandler(false);
    Context::Ptr pContext = new Context(Context::CLIENT_USE, "", "", "", 
      Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

    SSLManager::instance().initializeClient(0, pCert, pContext);

    SecureStreamSocket pSSLSocket(pContext);
    pSSLSocket.connect(SocketAddress(host, port));
    SecureSMTPClientSession session(pSSLSocket);

    session.login();
		if (!username.empty()) {
      // NOTE: The TLS May fail, or may succeed. It seems like there's no reason 
      // not to try if we're already SSL encrypted.
			session.startTLS(pContext);
			session.login(SMTPClientSession::AUTH_LOGIN, username, password);
		}

    session.sendMessage(*message);
    session.close();
  } else {
    // NOTE: This code path is untested. My ISP blocks outbound port 25, and I
    // don't really care to spend the time testing this.
		SMTPClientSession session(host, port); 

		session.login();
		if (!username.empty())
			session.login(SMTPClientSession::AUTH_LOGIN, username, password);
		session.sendMessage(*message);
		session.close();
  }

  return true;
}

bool SmtpNotifier::SendResults(nlohmann::json *results) {
  auto tmpl = *results;

  // TODO: These should go in the yaml under a template: section...
  // TODO: Add a thumbnail here. And if these aren't specified, they should default
  // to something in the template...
  tmpl["text_color"] = "#0D1B1E";
  tmpl["margin_color"] = "#C3DBC5";
  tmpl["border_color"] = "#7798AB";
  tmpl["body_color"] = "#E8DCB9";
  tmpl["alert_color"] = "#F2CEE6";

  tmpl["to"] = to;
  tmpl["from"] = from;
  tmpl["subject"] = subject;
  tmpl["template_html"] = template_html_path;

  // Compile the email :
	Poco::Net::MailMessage message;
	message.setSender(from);
	message.addRecipient(
		Poco::Net::MailRecipient(Poco::Net::MailRecipient::PRIMARY_RECIPIENT, to));
	message.setSubject(subject); // TODO: run the tmpl lib here

/* //TODO
	string notification_in_plain = "TODO: plain";
	message.setContentType("text/plain; charset=UTF-8");
	message.setContent(notification_in_plain, Poco::Net::MailMessage::ENCODING_8BIT);
*/

  inja::Environment env;
	
	env.add_callback("h", 1, [](inja::Arguments& args) {
    string ret;
		string s = args.at(0)->get<string>();

    ret.reserve(s.size());
    for(size_t pos = 0; pos != s.size(); ++pos) {
			switch(s[pos]) {
				case '&':  ret.append("&amp;");    break;
				case '\"': ret.append("&quot;");   break;
				case '\'': ret.append("&apos;");   break;
				case '<':  ret.append("&lt;");     break;
				case '>':  ret.append("&gt;");     break;
				default:   ret.append(&s[pos], 1); break;
			}
    }

		return ret;
	});

  auto notification_in_html = env.render_file(template_html_path, tmpl);

	Poco::Net::MediaType mediaType("multipart", "related");
	mediaType.setParameter("type", "text/html");
	message.setContentType(mediaType);

	message.addPart("", new Poco::Net::StringPartSource(notification_in_html, "text/html"), 
    Poco::Net::MailMessage::CONTENT_INLINE, 
      Poco::Net::MailMessage::ENCODING_QUOTED_PRINTABLE);

  // TODO: This should be inside add_callback
  Poco::Net::FilePartSource *image = new Poco::Net::FilePartSource(
    "views/images/home.jpg", "image/jpeg");
  image->headers().add("Content-ID", "<5e3424ee3db63_3c12aad4eea05bc88574@hostname.mail>");
  message.addPart("home.jpg", image, Poco::Net::MailMessage::CONTENT_ATTACHMENT, 
    Poco::Net::MailMessage::ENCODING_BASE64);

  return DeliverMessage(&message);
}


bool SmtpNotifier::PathIsReadable(string path) {
	filesystem::path p(path);

	error_code ec;
	auto perms = filesystem::status(p, ec).permissions();

	return ( (ec.value() == 0) && (
    (perms & filesystem::perms::owner_read) != filesystem::perms::none &&
    (perms & filesystem::perms::group_read) != filesystem::perms::none &&
    (perms & filesystem::perms::others_read) != filesystem::perms::none ) );
}
