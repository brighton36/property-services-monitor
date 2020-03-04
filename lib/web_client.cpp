#include "web_client.h"

#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"

using namespace std;
using namespace Poco::Net;

WebClient::WebClient(string address, int port, bool isSSL) {
  this->address = address;
  this->port = port;
  this->isSSL = isSSL; // TODO: Make this work
}

tuple<unsigned int, string> WebClient::get(string path) {
  // Integrate this in the monitor_service_web as well as below
  return make_tuple(500, path);
}

tuple<unsigned int, string> WebClient::post(string path, string body) {
  HTTPRequest request( HTTPRequest::HTTP_POST, path, HTTPMessage::HTTP_1_1 );

	request.setContentType("text/plain");
	request.setContentLength( body.length() );

  HTTPResponse res;
  stringstream ss;

  if (isSSL) {
    // TODO: It'd be nice to test this somehow... make we can make this branch
    // a private method, and call it from get and post....
    const Context::Ptr context = new Context( Context::CLIENT_USE, "", "", "", 
      Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

    HTTPSClientSession session(address, port, context);
    session.sendRequest(request) << body;
    Poco::StreamCopier::copyStream(session.receiveResponse(res), ss);
  } else {
    HTTPClientSession session(address, port);
    session.sendRequest(request) << body;
    Poco::StreamCopier::copyStream(session.receiveResponse(res), ss);
  }

	
	return make_tuple(res.getStatus(), ss.str());
}
