#include "monitor_job.h"

#include "fmt/printf.h"

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

NotifierSmtp::NotifierSmtp(string tpath, const YAML::Node config) {

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

	this->parameters = make_shared<unordered_map<string, string>>();
  
  if (config["parameters"])
    for(auto it=config["parameters"].begin();it!=config["parameters"].end();++it) {
      const auto param = it->first.as<std::string>();
      const auto value = it->second.as<std::string>();
      
      this->parameters->insert(make_pair(param, value));
    }
}

bool NotifierSmtp::DeliverMessage(MailMessage *message) {
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

unique_ptr<inja::Environment> NotifierSmtp::GetInjaEnv() {
  unique_ptr<inja::Environment> env(new inja::Environment);

	env->add_callback("h", 1, [](inja::Arguments& args) {
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

	env->add_callback("f", 2, [](inja::Arguments& args) {
		string formatter = args.at(0)->get<string>();
    auto p = args.at(1);
    
    string ret;

    switch ( p->type() ) {
      case nlohmann::json::value_t::string : 
        ret = fmt::sprintf(formatter, p->get<string>());
        break;
      case nlohmann::json::value_t::number_unsigned : 
        ret = fmt::sprintf(formatter, p->get<unsigned int>());
        break;
      case nlohmann::json::value_t::number_integer : 
        ret = fmt::sprintf(formatter, p->get<int>());
        break;
      case nlohmann::json::value_t::number_float : 
        ret = fmt::sprintf(formatter, p->get<float>());
        break;
      case nlohmann::json::value_t::boolean : 
        ret = fmt::sprintf(formatter, p->get<bool>());
        break;
      default: 
        inja::inja_throw("render_error", "Unrecognized value type passed to f()");
    }

    return ret; 
  });
  
  return env;
}

bool NotifierSmtp::SendResults(nlohmann::json *results) {
  auto tmpl = *results;

  for (auto param : *parameters) tmpl[param.first] = param.second;

  tmpl["to"] = to;
  tmpl["from"] = from;
  tmpl["subject"] = subject;

  auto unix_now = time(nullptr);
  auto local_now = localtime( &unix_now );

  tmpl["now"] = nlohmann::json::object();
  tmpl["now"]["sec"]   = local_now->tm_sec;
  tmpl["now"]["min"]   = local_now->tm_min;
  tmpl["now"]["hour"]  = local_now->tm_hour;
  tmpl["now"]["mday"]  = local_now->tm_mday;
  tmpl["now"]["mon"]   = (1+local_now->tm_mon);
  tmpl["now"]["year"]  = (1900+local_now->tm_year);
  tmpl["now"]["wday"]  = local_now->tm_wday;
  tmpl["now"]["yday"]  = local_now->tm_yday;
  tmpl["now"]["isdst"] = local_now->tm_isdst;
  tmpl["now"]["zone"]  = local_now->tm_zone;

  // Compile the email :
	MailMessage message;
	message.setSender(from);
	message.addRecipient(
		MailRecipient(MailRecipient::PRIMARY_RECIPIENT, to));

  auto inja = GetInjaEnv();

  cout << "Sizeof env:" << sizeof(inja) << endl;

	message.setSubject(subject); // TODO: run the tmpl lib here

  auto notification_in_html = inja->render_file(template_html_path, tmpl);
  auto notification_in_plain = inja->render_file(template_plain_path, tmpl);

	message.setContentType("text/plain; charset=UTF-8");
	message.setContent(notification_in_plain, MailMessage::ENCODING_8BIT);

	MediaType mediaType("multipart", "related");
	mediaType.setParameter("type", "text/html");
	message.setContentType(mediaType);

	message.addPart("", new StringPartSource(notification_in_html, "text/html"), 
    MailMessage::CONTENT_INLINE, MailMessage::ENCODING_QUOTED_PRINTABLE);

  // TODO: This should be inside add_callback
  FilePartSource *image = new FilePartSource(
    "views/images/home.jpg", "image/jpeg");
  image->headers().add("Content-ID", "<5e3424ee3db63_3c12aad4eea05bc88574@hostname.mail>");
  message.addPart("home.jpg", image, MailMessage::CONTENT_ATTACHMENT, 
    MailMessage::ENCODING_BASE64);

  return DeliverMessage(&message);
}


bool NotifierSmtp::PathIsReadable(string path) {
	filesystem::path p(path);

	error_code ec;
	auto perms = filesystem::status(p, ec).permissions();

	return ( (ec.value() == 0) && (
    (perms & filesystem::perms::owner_read) != filesystem::perms::none &&
    (perms & filesystem::perms::group_read) != filesystem::perms::none &&
    (perms & filesystem::perms::others_read) != filesystem::perms::none ) );
}
