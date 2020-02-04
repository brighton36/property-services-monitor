#include "monitor_job.h"

#include "fmt/printf.h"

#include "Poco/SHA1Engine.h"
#include "Poco/Crypto/RSAKey.h"
#include "Poco/Crypto/RSADigestEngine.h"

#include "Poco/Net/MailRecipient.h"
#include "Poco/Net/SMTPClientSession.h"
#include "Poco/Net/SecureSMTPClientSession.h"
#include "Poco/Net/InvalidCertificateHandler.h"
#include "Poco/Net/AcceptCertificateHandler.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/SecureStreamSocket.h"

#include <Poco/Net/StringPartSource.h>
#include <Poco/Net/MediaType.h>

#define CANT_READ "Unable to open file {}."
#define DEFAULT_TEMPLATE_SUBJECT "{{subject}}{% if has_failures %}: ATTENTION REQUIRED{% endif %}"

using namespace std;
using namespace Poco::Net;
using namespace Poco::Crypto;

SmtpAttachment::SmtpAttachment(string base_path, string file_path) {
	SetFilepath( (filesystem::path(file_path).is_relative()) ? 
    fmt::format("{}/{}", base_path, file_path) : file_path );

  if (!PathIsReadable(full_path)) 
    throw invalid_argument(fmt::format(CANT_READ, full_path));

	// Read the file into a buffer:
  auto &is = file_part_source->stream();
	is.seekg(0, is.end);
	int length = is.tellg();
  if (length <= 0) throw invalid_argument(fmt::format(CANT_READ, full_path));
	is.seekg(0, is.beg);
	auto buffer = new char [length];
	is.read(buffer,length);

	// Now let's get the hash of this file's contents:
	RSADigestEngine eng(RSAKey(RSAKey::KL_2048, RSAKey::EXP_LARGE), "SHA256");

	eng.update(buffer,length);
	contents_hash = Poco::DigestEngine::digestToHex(eng.digest());

	delete buffer;
}

SmtpAttachment::SmtpAttachment(const SmtpAttachment &s2) {
  SetFilepath(s2.full_path);
	contents_hash = s2.contents_hash;
}

bool SmtpAttachment::operator==(const SmtpAttachment &s2) {
  return (contents_hash == s2.contents_hash);
}

void SmtpAttachment::SetFilepath(std::string f) {
	full_path = f;

	// Let's figure out what kind of file it is based off the extension:
  std::smatch matches;

  if (!regex_search(full_path, matches, regex("([^\\.]+)$")) || (matches.size() != 2))
    throw invalid_argument(fmt::format("Unable to find file extension for {}", GetFilename()));

  string file_ext = matches[1].str();
  
  if (("jpg" == file_ext) || ("jpeg" == file_ext)) file_mime_type = "image/jpeg";
  else if ("png" == file_ext) file_mime_type = "image/png";
  else if ("gif" == file_ext) file_mime_type = "image/gif";
  else if ("webp" == file_ext) file_mime_type = "image/webp";
  else if ("webm" == file_ext) file_mime_type = "image/webm";
  else
    throw invalid_argument(fmt::format("Unable to find mime type for {}", GetFilename()));

  // Create the FilePartSource:
  file_part_source = make_unique<FilePartSource>(full_path, file_mime_type);
}

string SmtpAttachment::GetContentID() { 
  return fmt::format("{}@hostname.mail", contents_hash);
}

string SmtpAttachment::GetFilename() { 
  return filesystem::path(full_path).filename();
}

void SmtpAttachment::AttachToMessage(MailMessage *m) { 
  // This needs to go roughly here, as the hash may not be generated until 
  // construction ends. Note that GetContentID is largely the contents_hash:
  file_part_source->headers().add("Content-ID", fmt::format("<{}>", GetContentID()));

  m->addPart(GetFilename(), file_part_source.release(),
    MailMessage::CONTENT_ATTACHMENT, MailMessage::ENCODING_BASE64);
}


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

  template_subject = (config["template_subject"]) ? 
    config["template_subject"].as<string>() : DEFAULT_TEMPLATE_SUBJECT;

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

  attachments.clear();

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

    switch ( p->type() ) {
      case nlohmann::json::value_t::string : 
        return fmt::sprintf(formatter, p->get<string>());
        break;
      case nlohmann::json::value_t::number_unsigned : 
        return fmt::sprintf(formatter, p->get<unsigned int>());
        break;
      case nlohmann::json::value_t::number_integer : 
        return fmt::sprintf(formatter, p->get<int>());
        break;
      case nlohmann::json::value_t::number_float : 
        return fmt::sprintf(formatter, p->get<float>());
        break;
      case nlohmann::json::value_t::boolean : 
        return fmt::sprintf(formatter, p->get<bool>());
        break;
      default: 
        break;
    }

    inja::inja_throw("render_error", "Unrecognized value type passed to f()");

    return string();
  });

	env->add_callback("image_src", 1, [&](inja::Arguments& args) {
    auto attachment = SmtpAttachment(base_path, args.at(0)->get<string>());
    
    // If it's not already in the attachments vector, add it:
    if(find(attachments.begin(), attachments.end(), attachment) == attachments.end())
      attachments.push_back(attachment);

    return fmt::format("cid:{}", attachment.GetContentID());
  });
  
  return env;
}

nlohmann::json NotifierSmtp::GetNow() {
  auto ret = nlohmann::json::object();
  auto unix_now = time(nullptr);
  auto local_now = localtime( &unix_now );

  ret["sec"]   = local_now->tm_sec;
  ret["min"]   = local_now->tm_min;
  ret["hour"]  = local_now->tm_hour;
  ret["mday"]  = local_now->tm_mday;
  ret["mon"]   = (1+local_now->tm_mon);
  ret["year"]  = (1900+local_now->tm_year);
  ret["wday"]  = local_now->tm_wday;
  ret["yday"]  = local_now->tm_yday;
  ret["isdst"] = local_now->tm_isdst;
  ret["zone"]  = local_now->tm_zone;

  return ret;
}

bool NotifierSmtp::SendResults(nlohmann::json *results) {
  auto tmpl = *results;

  for (auto param : *parameters) tmpl[param.first] = param.second;

  tmpl["to"] = to;
  tmpl["from"] = from;
  tmpl["subject"] = subject;

  tmpl["now"] = GetNow();

  // Compile the email :
	MailMessage message;
	message.setSender(from);
	message.addRecipient(MailRecipient(MailRecipient::PRIMARY_RECIPIENT, to));

  auto inja = GetInjaEnv();

	message.setSubject(inja->render(template_subject, tmpl));

  auto notification_in_html = inja->render_file(template_html_path, tmpl);
  auto notification_in_plain = inja->render_file(template_plain_path, tmpl);

	message.setContentType("text/plain; charset=UTF-8");
	message.setContent(notification_in_plain, MailMessage::ENCODING_8BIT);

	MediaType mediaType("multipart", "related");
	mediaType.setParameter("type", "text/html");
	message.setContentType(mediaType);

	message.addPart("", new StringPartSource(notification_in_html, "text/html"), 
    MailMessage::CONTENT_INLINE, MailMessage::ENCODING_QUOTED_PRINTABLE);

  for (auto& attachment : attachments) 
    attachment.AttachToMessage(&message);

  return DeliverMessage(&message);
}


