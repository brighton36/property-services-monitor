#include "web_client.h"

#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"

#include <iostream> // TODO: remove

using namespace std;
using namespace Poco::Net;

WebClient::WebClient(string address, int port, bool isSSL) {
  this->address = address;
  this->port = port;
  this->isSSL = isSSL;
}

// TODO: Pass params By references....
TUPLE_INT_STR WebClient::req(Poco::Net::HTTPRequest *request, string body, MAP_STR_STR headers) {
  HTTPResponse res;
  stringstream ss;

	for(MAP_STR_STR::iterator it = headers.begin(); it != headers.end(); it++)
    request->set(it->first, it->second);

  // TODO: Check request.type instead merbe. Yeah, rather that take a request, take a HTTPREquest::HTTP_*
  if (!body.empty()) request->setContentLength( body.length() );

  if (isSSL) {
    const Context::Ptr context = new Context( Context::CLIENT_USE, "", "", "", 
      Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

    HTTPSClientSession session(address, port, context);
    std::ostream& os = session.sendRequest(*request);

    if (!body.empty()) os << body;

    Poco::StreamCopier::copyStream(session.receiveResponse(res), ss);
  } else {
    HTTPClientSession session(address, port);
    std::ostream& os = session.sendRequest(*request);

    if (!body.empty()) os << body;

    Poco::StreamCopier::copyStream(session.receiveResponse(res), ss);
  }
	
	return make_tuple(res.getStatus(), ss.str());
}

TUPLE_INT_STR WebClient::get(string path, MAP_STR_STR headers) {
  HTTPRequest request( HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1 );
	return this->req(&request, string(), headers);
}


TUPLE_INT_STR WebClient::post(string path, string body, MAP_STR_STR headers) {
  HTTPRequest request( HTTPRequest::HTTP_POST, path, HTTPMessage::HTTP_1_1 );
	return this->req(&request, body, headers);
}

