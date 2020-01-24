#include "monitor_job.h"

#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"

using namespace std;

ServiceRegister<MonitorServiceWeb> MonitorServiceWeb::reg("web");

MonitorServiceWeb::MonitorServiceWeb(string address, PTR_UMAP_STR params) 
  : MonitorServiceBase("web", address, params) {

  // Default values:
  this->isHttps = false;
  this->port = 0;
  this->path = "/";
  this->status_equals = Poco::Net::HTTPResponse::HTTP_OK;

  for( const auto& n : *params )
    if ("proto" == n.first) {
			if (n.second == "http")
				this->isHttps = false;
      else if (n.second == "https")
				this->isHttps = true;
			else
				throw invalid_argument(fmt::format("Unrecognized web proto \"{}\".", 
					n.second));
    }
    else if ("port" == n.first)
      this->port = stoi(n.second);
    else if ("path" == n.first)
      this->path = n.second;
    else if ("ensure_match" == n.first)
      // TODO : It'd be nice to support a //i type thing...
      // We probably need to store this as  a string and test it here. Then, roll
      // it out in the method later
      this->ensure_match = regex(n.second);
    else
      throw invalid_argument(fmt::format("Unrecognized ping parameter \"{}\".", 
        n.first));

  if (!this->port) this->port = (this->isHttps) ? 443 : 80;
}

string MonitorServiceWeb::HttXRequest(string path, Poco::Net::HTTPResponse &response) {

  string content;

  Poco::Net::HTTPRequest request( Poco::Net::HTTPRequest::HTTP_GET, 
    path, Poco::Net::HTTPMessage::HTTP_1_1 );

  if (this->isHttps) {
    const Poco::Net::Context::Ptr context = new Poco::Net::Context(
      Poco::Net::Context::CLIENT_USE, "", "", "",
      Poco::Net::Context::VERIFY_NONE, 9, false,
      "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
    Poco::Net::HTTPSClientSession session(this->address, this->port, context);
    session.sendRequest(request);

    Poco::StreamCopier::copyToString(session.receiveResponse(response), content);
    return content;
  } else {
    Poco::Net::HTTPClientSession session(this->address, this->port);
    session.sendRequest(request);

    Poco::StreamCopier::copyToString(session.receiveResponse(response), content);
    return content;
  }
}

bool MonitorServiceWeb::IsAvailable() {

	try {
    this->results->clear(); // TODO: put this in the parent... and set initial params there

		Poco::Net::HTTPResponse response;

    string response_content = this->HttXRequest(this->path, response);

    this->results->emplace("response_status", to_string(response.getStatus()));
    this->results->emplace("response_reason", response.getReason());
    this->results->emplace("response_content", response_content);

    if (response.getStatus() == this->status_equals) {
      // TODO: Test if we were passed a regex, and only then Grep it for things.
      smatch m;
      regex_search(response_content, m, this->ensure_match);

      for(auto v: m)
        std::cout << "Match:" << v << std::endl;

      // TODO Ensure match with returen

      return true;
    }
    else 
      // TODO: Capture the output status and code
      return false;
	}
	catch (Poco::Exception& exc) { 
    cout << "Exception:" << exc.what() << endl;
    return false; 
  }
}
