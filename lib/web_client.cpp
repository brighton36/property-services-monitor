#include "web_client.h"

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"

using namespace std;
using namespace Poco::Net;

WebClient::WebClient(string address, int port, bool isSSL) {
  this->address = address;
  this->port = port;
  this->isSSL = isSSL;
}

TUPLE_INT_STR WebClient::req(string method, string *path, string *body, MAP_STR_STR *headers) {
  HTTPResponse res;
  stringstream ss;
  
  HTTPRequest request( method, *path, HTTPMessage::HTTP_1_1 );

  if (headers != nullptr)
    for(MAP_STR_STR::iterator it = headers->begin(); it != headers->end(); it++)
      request.set(it->first, it->second);

  if (body != nullptr) request.setContentLength( body->length() );

  if (isSSL) {
    const Context::Ptr context = new Context( Context::CLIENT_USE, "", "", "", 
      Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

    HTTPSClientSession session(address, port, context);
    std::ostream& os = session.sendRequest(request);

    if (body != nullptr) os << *body;

    Poco::StreamCopier::copyStream(session.receiveResponse(res), ss);
  } else {
    HTTPClientSession session(address, port);
    std::ostream& os = session.sendRequest(request);

    if (body != nullptr) os << *body;

    Poco::StreamCopier::copyStream(session.receiveResponse(res), ss);
  }
	
	return make_tuple(res.getStatus(), ss.str());
}

TUPLE_INT_STR WebClient::get(std::string path) { 
	return req(HTTPRequest::HTTP_GET, &path, nullptr, nullptr);
}

TUPLE_INT_STR WebClient::get(string path, MAP_STR_STR headers) {
	return req(HTTPRequest::HTTP_GET, &path, nullptr, &headers);
}

TUPLE_INT_STR WebClient::post(std::string path, std::string body) {
	return req(HTTPRequest::HTTP_POST, &path, &body, nullptr);
}
TUPLE_INT_STR WebClient::post(string path, string body, MAP_STR_STR headers) {
	return req(HTTPRequest::HTTP_POST, &path, &body, &headers);
}

