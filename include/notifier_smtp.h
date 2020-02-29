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

class SmtpAttachment {
  public:
    std::string full_path, contents_hash, file_mime_type;
    std::unique_ptr<Poco::Net::FilePartSource> file_part_source;
    SmtpAttachment(std::string);
    SmtpAttachment(const SmtpAttachment &);
    bool operator==(const SmtpAttachment &);
    std::string getFilename();
    std::string getContentID();
    void attachTo(Poco::Net::MailMessage *);
  private:
    void setFilepath(std::string);
};

class NotifierSmtp { 
  public:
    std::string to, from, subject, host, username, password, base_path;
    std::string template_subject, template_html_path, template_plain_path;
    unsigned int port;
    bool isSSL;
    PTR_MAP_STR_STR parameters;
    std::unique_ptr<inja::Environment> inja;
    std::vector<SmtpAttachment> attachments;

    NotifierSmtp(std::string, const YAML::Node);
    NotifierSmtp() {};
    bool sendResults(nlohmann::json*);
    bool deliverMessage(Poco::Net::MailMessage *);
    static std::string Help();
  private:
    nlohmann::json getNow();
    std::string current_template_path;
    std::string getDefaultTemplatePath();
    std::string toFullPath(std::string, std::string);
    std::unique_ptr<inja::Environment> getInjaEnv();
    std::string renderHtml(const std::string, const nlohmann::json *);
    std::string renderPlain(const std::string, const nlohmann::json *);
};

