#include "property-services-monitor.h"

#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"

using namespace std;

ServiceRegister<MonitorServiceWeb> MonitorServiceWeb::reg("web");

std::string MonitorServiceWeb::Help() {
  return fmt::format(
    " * proto          (optional) Either \"http\" or \"https\". Defaults to \"http\".\n"
    " * port           (optional) The port number of the http service. Defaults to 80 (http) or\n"
		"                             443 (https), depending on the proto value.\n"
    " * path           (optional) The resource path being request from the http server. Defaults\n"
		"                             to \"/\".\n"
    " * status_equals  (optional) The expected HTTP status code, to be received from the server.\n"
		"                             Defaults to {}.\n"
    " * ensure_match   (optional) A regular expression to be found in the return content body.\n"
		"                             This regex is expected to be in the C++ regex format (no /'s).\n", 
		Poco::Net::HTTPResponse::HTTP_OK );
}

MonitorServiceWeb::MonitorServiceWeb(string address, PTR_MAP_STR_STR params) 
  : MonitorServiceBase("web", address, params) {

  // Default values:
  isHttps = false;
  port = 0;
  path = "/";
  status_equals = Poco::Net::HTTPResponse::HTTP_OK;

  for( const auto& n : *params )
    if ("proto" == n.first) {
			if (n.second == "http")
				isHttps = false;
      else if (n.second == "https")
				isHttps = true;
			else
				throw invalid_argument(fmt::format("Unrecognized web proto \"{}\".", 
					n.second));
    }
    else if ("port" == n.first)
      port = stoi(n.second);
    else if ("path" == n.first)
      path = n.second;
    else if ("status_equals" == n.first)
      status_equals = stoi(n.second);
    else if ("ensure_match" == n.first)
      ensure_match = n.second;
    else
      throw invalid_argument(fmt::format("Unrecognized ping parameter \"{}\".", 
        n.first));

  if (!port) port = (isHttps) ? 443 : 80;
}

string MonitorServiceWeb::httxRequest(string path, Poco::Net::HTTPResponse &response) {

  string content;

  Poco::Net::HTTPRequest request( Poco::Net::HTTPRequest::HTTP_GET, 
    path, Poco::Net::HTTPMessage::HTTP_1_1 );

  if (isHttps) {
    const Poco::Net::Context::Ptr context = new Poco::Net::Context(
      Poco::Net::Context::CLIENT_USE, "", "", "",
      Poco::Net::Context::VERIFY_NONE, 9, false,
      "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
    Poco::Net::HTTPSClientSession session(address, port, context);
    session.sendRequest(request);

    Poco::StreamCopier::copyToString(session.receiveResponse(response), content);
    return content;
  } else {
    Poco::Net::HTTPClientSession session(address, port);
    session.sendRequest(request);

    Poco::StreamCopier::copyToString(session.receiveResponse(response), content);
    return content;
  }
}

bool MonitorServiceWeb::isAvailable() {
  MonitorServiceBase::isAvailable();

	try {
		Poco::Net::HTTPResponse response;

    string response_content = httxRequest(path, response);

    resultAdd("response_status", to_string(response.getStatus()));
    resultAdd("response_reason", response.getReason());
    resultAdd("response_content", response_content);

    auto status_code = response.getStatus();

    if (status_code == status_equals) {
      if (!ensure_match.empty()) {
        auto re = regex(ensure_match);

        auto match_count = distance(
          sregex_iterator( response_content.begin(), response_content.end(), re), 
          sregex_iterator());

        resultAdd("response_match_count", to_string(match_count));

        if ( match_count == 0 )
          return resultFail("Unable to find {} in content response", ensure_match);
      }

      return true;
    }
    else
      return resultFail("Server status code {} did not match the expected {} status code.", 
          status_code, status_equals);
	}
	catch (const exception& e) { 
    return resultFail(e.what());
  }
}
