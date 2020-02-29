#include "notifier_smtp.h"

using namespace std;
using namespace Poco::Net;
using namespace Poco::Crypto;

SmtpAttachment::SmtpAttachment(string file_path) {
  setFilepath(file_path);

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
