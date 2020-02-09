#include "property-services-monitor.h"

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
	setFilepath( (filesystem::path(file_path).is_relative()) ? 
    fmt::format("{}/{}", base_path, file_path) : file_path );

  if (!pathIsReadable(full_path)) 
    throw invalid_argument(fmt::format(CANT_READ, full_path));

	// Read the file into a buffer:
  auto &is = file_part_source->stream();
	is.seekg(0, is.end);
	int length = is.tellg();
  if (length <= 0) throw invalid_argument(fmt::format(CANT_READ, full_path));
	is.seekg(0, is.beg);
	auto buffer = make_unique<char[]>(length);
	is.read(buffer.get(),length);

	// Now let's get the hash of this file's contents:
	RSADigestEngine eng(RSAKey(RSAKey::KL_2048, RSAKey::EXP_LARGE), "SHA256");

	eng.update(buffer.get(),length);
	contents_hash = Poco::DigestEngine::digestToHex(eng.digest());
}

SmtpAttachment::SmtpAttachment(const SmtpAttachment &s2) {
  setFilepath(s2.full_path);
	contents_hash = s2.contents_hash;
}

bool SmtpAttachment::operator==(const SmtpAttachment &s2) {
  return (contents_hash == s2.contents_hash);
}

void SmtpAttachment::setFilepath(std::string f) {
	full_path = f;

	// Let's figure out what kind of file it is based off the extension:
  std::smatch matches;

  if (!regex_search(full_path, matches, regex("([^\\.]+)$")) || (matches.size() != 2))
    throw invalid_argument(fmt::format("Unable to find file extension for {}", getFilename()));

  string file_ext = matches[1].str();
  
  if (("jpg" == file_ext) || ("jpeg" == file_ext)) file_mime_type = "image/jpeg";
  else if ("png" == file_ext) file_mime_type = "image/png";
  else if ("gif" == file_ext) file_mime_type = "image/gif";
  else if ("webp" == file_ext) file_mime_type = "image/webp";
  else if ("webm" == file_ext) file_mime_type = "image/webm";
  else
    throw invalid_argument(fmt::format("Unable to find mime type for {}", getFilename()));

  // Create the FilePartSource:
  file_part_source = make_unique<FilePartSource>(full_path, file_mime_type);
}

string SmtpAttachment::getContentID() { 
  return fmt::format("{}@hostname.mail", contents_hash);
}

string SmtpAttachment::getFilename() { 
  return filesystem::path(full_path).filename();
}

void SmtpAttachment::attachTo(MailMessage *m) { 
  // This needs to go roughly here, as the hash may not be generated until 
  // construction ends. Note that getContentID is largely the contents_hash:
  file_part_source->headers().add("Content-ID", fmt::format("<{}>", getContentID()));

  m->addPart(getFilename(), file_part_source.release(),
    MailMessage::CONTENT_ATTACHMENT, MailMessage::ENCODING_BASE64);
}

std::string NotifierSmtp::Help() {
  return "The \"notification\" (map) supports the following parameters:\n"
    " * to               (required) A valid SMTP address to which the notification will be delivered.\n"
    " * from             (required) A valid SMTP address from which the notification will be address.\n"
    " * subject          (required) The Subject line of the e-mail notification.\n"
    " * host             (required) The fqdn or ip address of the smtp server to use for relaying.\n"
    " * template_html    (optional) A relative or absolute path to an inja template, used \n"
    "                               for constructing the email html body\n"
    " * template_plain   (optional) A relative or absolute path to an inja template, used \n"
    "                               for constructing the email plain text body\n"    
    " * template_subject (optional) An inja string used to format the smtp subject line\n"
    " * proto            (optional) Either \"plain\" or \"ssl\". Defaults to \"plain\".\n"
    " * port             (optional) The port number of the smtp relay server. Defaults to 25 (plain)\n"
    "                               or 465 (ssl), depending on the \"proto\" value. \n"
    " * username         (optional) The login credential for the smtp relay server.\n"
    " * password         (optional) The password credential for the smtp relay server.\n"
    " * parameters       (optional) A key to value (map). These parameters are passed to the template\n"
    "                               files, as specified. See the included template files for what values\n"
    "                               are supported here by the shipped templates. Feel free to add any\n"
    "                               values of your own.\n";
}

NotifierSmtp::NotifierSmtp(string tpath, const YAML::Node config) {

  if (!config["to"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "to"));
  if (!config["from"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "from"));
  if (!config["subject"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "subject"));
  if (!config["host"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "host"));

  base_path = tpath;
  to = config["to"].as<string>();
  from = config["from"].as<string>();
  subject = config["subject"].as<string>();

  string executable_path = filesystem::path(
    filesystem::read_symlink("/proc/self/exe")).parent_path();

  template_html_path = (config["template_html"]) ? 
    toFullPath(base_path, config["template_html"].as<string>()) :
    toFullPath(executable_path, "views/notify.html.inja");

  template_plain_path = (config["template_plain"]) ? 
    toFullPath(base_path, config["template_plain"].as<string>()) :
    toFullPath(executable_path, "views/notify.plain.inja");

  template_subject = (config["template_subject"]) ? 
    config["template_subject"].as<string>() : DEFAULT_TEMPLATE_SUBJECT;

  if (!pathIsReadable(template_html_path)) 
    throw invalid_argument(fmt::format(CANT_READ, template_html_path));
  if (!pathIsReadable(template_plain_path)) 
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

	parameters = make_shared<map<string, string>>();
  
  if (config["parameters"])
    for(auto it=config["parameters"].begin();it!=config["parameters"].end();++it) {
      const auto param = it->first.as<std::string>();
      const auto value = it->second.as<std::string>();
      
      parameters->insert(make_pair(param, value));
    }
}

// If file isn't a full path, we make it one, using the provided base
string NotifierSmtp::toFullPath(string base, string file) {
  return (filesystem::path(file).is_relative()) ? 
    fmt::format("{}/{}", base, file) : file;
}

bool NotifierSmtp::deliverMessage(MailMessage *message) {
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

unique_ptr<inja::Environment> NotifierSmtp::getInjaEnv() {

  attachments.clear();

  auto env = make_unique<inja::Environment>();

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
    // TODO: This should be canonical template path
    auto attachment = SmtpAttachment(base_path, args.at(0)->get<string>());
    
    // If it's not already in the attachments vector, add it:
    if(find(attachments.begin(), attachments.end(), attachment) == attachments.end())
      attachments.push_back(attachment);

    return fmt::format("cid:{}", attachment.getContentID());
  });
  
  return env;
}

nlohmann::json NotifierSmtp::getNow() {
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

bool NotifierSmtp::sendResults(nlohmann::json *results) {
  auto tmpl = *results;

  for (auto param : *parameters) tmpl[param.first] = param.second;

  tmpl["to"] = to;
  tmpl["from"] = from;
  tmpl["subject"] = subject;

  tmpl["now"] = getNow();

  // Compile the email :
	MailMessage message;
	message.setSender(from);
	message.addRecipient(MailRecipient(MailRecipient::PRIMARY_RECIPIENT, to));

  auto inja = getInjaEnv();

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

  for (auto& attachment : attachments) attachment.attachTo(&message);

  return deliverMessage(&message);
}


