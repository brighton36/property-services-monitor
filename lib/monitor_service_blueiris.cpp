#include "monitor_service_blueiris.h"

#include <ctime>
#include <chrono>

#include "Poco/MD5Engine.h"
#include "Poco/DigestStream.h"

using namespace std;
using namespace nlohmann;
using namespace Poco::Net;

WebClient::WebClient(string address, int port, bool isSSL) {
  this->address = address;
  this->port = port;
  this->isSSL = isSSL; // TODO: Make this work
}

// TODO: I think we want to return a response and a string pair
string WebClient::get(string path) {
  // Integrate this in the monitor_service_web as well as below
  cout << path << endl;
  return "TODO";
}

// TODO: I think we want to return a response and a string pair
string WebClient::post(string path, string body) {
  Poco::Net::HTTPRequest request( Poco::Net::HTTPRequest::HTTP_POST, path, HTTPMessage::HTTP_1_1 );

	request.setContentType("text/plain");

	request.setContentLength( body.length() );

  HTTPClientSession session(address, port);

  ostream& os = session.sendRequest(request);

	os << body;

  HTTPResponse res;
  cout << res.getStatus() << " " << res.getReason() << endl;

  istream &is = session.receiveResponse(res);

	stringstream ss;
	Poco::StreamCopier::copyStream(is, ss);
	
	return ss.str();
}


//////////////////////////////////
ServiceRegister<MonitorServiceBlueIris> MonitorServiceBlueIris::reg("blueiris");

std::string MonitorServiceBlueIris::Help() { 
  return "TODO";
}

MonitorServiceBlueIris::MonitorServiceBlueIris(string address, PTR_MAP_STR_STR params) 
  : MonitorServiceBase("blueiris", address, params) { 
  // TODO : Params 
  
  unsigned int port = 81;
  bool isSSL = false;
  session = string();

  setParameters({
    {"port",     [&](string v) { port = stoi(v);}},
    {"username", [&](string v) { username = v;}},
    {"password", [&](string v) { password = v;} },
    {"proto",    [&](string v) { 
      if (v == "http")
        isSSL = false;
      else if (v == "https")
        isSSL = true;
      else
        throw invalid_argument(fmt::format("Unrecognized web proto \"{}\".", v));
    } }
  });

  // tODO: Username and pass are required. Fail if omitted
  client = make_unique<WebClient>(address, port, isSSL);
}

json MonitorServiceBlueIris::sendCommand(string command, json options = json()) {
  string response;

  if (session.empty()) {
    // Let's login/create a session
    string first_response;
    response = client->post("/json", json( {{"cmd", "login"}} ).dump());

    // TODO: Check for 200 code
    auto first_json = json::parse(response);

    auto proposed_session = string(first_json["session"]);

    string auth_content = fmt::format("{}:{}:{}", username, proposed_session, password);

    Poco::MD5Engine eng;
    eng.update(auth_content);
    string login_response = Poco::DigestEngine::digestToHex(eng.digest());

    response = client->post("/json", json({ 
      {"cmd", "login"}, {"session", proposed_session}, {"response", login_response} 
    }).dump());
    // TODO: Check for 200 code

    cout << "Authenticated OK:" << response << endl;
    session = proposed_session;
  }

  options["session"] = session;
  options["cmd"] = command;

  response = client->post("/json", options.dump());

  // TODO: Check for 200 code
  // TODO: Check for response["result"] equalling "success"
  // TODO: Check for response["session"] equalling what we have
  return json::parse(response);
}

bool MonitorServiceBlueIris::isAvailable() {
  MonitorServiceBase::isAvailable();

  // TODO : try/Catch errors
  /*
  json response; 

  response = sendCommand("status");

	cout << "Status:" << response.dump() << endl;

  response = sendCommand("log");

	cout << "Log:" << response.dump() << endl;
  */

  // TODO: let's make use of startdate and index via a param
  json alerts = sendCommand("alertlist", {{"camera", "Index"}});

	cout << "Alerts:" << endl;
  // TODO: What to do about the zone... and let's maybe put this into a getAlerts() function
	for (auto& [i, alert] : alerts["data"].items()) {
    // TODO: auto bi_alert = BlueIrisAlert(alert);

		time_t date = static_cast<time_t>(alert["date"]);
		char date_string[80];

		strftime(date_string,80,"%Y-%m-%d %H:%M",localtime(&date));

		cout << i << " : " << alert["camera"] << " : " << string(date_string) << endl;
	}

	//cout << "alert list:" << alerts.dump() << endl;

  long int now = static_cast<long int>(time(nullptr));
  cout << fmt::format("Now: {}",now) << endl;

  return true;
}
