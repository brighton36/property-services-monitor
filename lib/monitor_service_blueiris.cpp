#include "monitor_service_blueiris.h"

#include <ctime>

#include "Poco/MD5Engine.h"
#include "Poco/DigestStream.h"

using namespace std;
using namespace nlohmann;

ServiceRegister<MonitorServiceBlueIris> MonitorServiceBlueIris::reg("blueiris");

std::string MonitorServiceBlueIris::Help() { 
  return "TODO";
}

MonitorServiceBlueIris::MonitorServiceBlueIris(string address, PTR_MAP_STR_STR params) 
  : MonitorServiceBase("blueiris", address, params) { 
  unsigned int port = 0;
  bool isSSL = false;
  session = string();
  capture_from = "Yesterday";
  capture_camera = "Index";

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
    } },
    {"capture_from", [&](string v) { capture_from = v;}},
    {"capture_to",   [&](string v) { capture_to = v;}},
    {"capture_camera",   [&](string v) { capture_camera = v;}},
  } );

  if (username.empty()) throw invalid_argument("Missing required field \"username\".");
  if (password.empty()) throw invalid_argument("Missing required field \"password\".");
 
  if (!port) port = (isSSL) ? 443 : 81;

  client = make_unique<WebClient>(address, port, isSSL);
}

json MonitorServiceBlueIris::sendCommand(string command, json options = json()) {
  string response;
  unsigned int code;
  json json_response;

  if (session.empty()) {
    // Let's login & create a session:
    string first_response;

    tie(code, response) = client->post("/json", json( {{"cmd", "login"}} ).dump());

    if (code != 200)
      throw BlueIrisException("Error code {} when requesting login page.", code);

    string proposed_session = json::parse(response)["session"].get<string>();

    Poco::MD5Engine eng;
    eng.update(fmt::format("{}:{}:{}", username, proposed_session, password));

    tie(code, response) = client->post("/json", json({ 
      {"cmd", "login"}, {"session", proposed_session}, 
      {"response", Poco::DigestEngine::digestToHex(eng.digest())} 
    }).dump());

    if (code != 200)
      throw BlueIrisException("Error code {} in response to authentication request.", code);

    json_response = json::parse(response);

    if (json_response["result"] != "success")
      throw BlueIrisException("Error Result {} in response to authentication request.", 
        json_response["result"]);

    // Authentication Suceeded. Set our session code so that we don't authenticate
    // again.
    session = proposed_session;
  }

  options["session"] = session;
  options["cmd"] = command;

  tie(code, response) = client->post("/json", options.dump());

  json_response = json::parse(response);
  if (code != 200)
    throw BlueIrisException("Error code {} in response to {} command.", code, command);

  if (json_response["result"] != "success")
    throw BlueIrisException("Error Result {} in response to {} command.", 
      json_response["result"], command);

  return json_response["data"];
}

shared_ptr<vector<BlueIrisAlert>> 
MonitorServiceBlueIris::getAlerts(time_t since = 0, string camera = "Index") {
  auto ret = make_shared<vector<BlueIrisAlert>>();

  json options = {{"camera", camera}};

  if (since > 0) options["startdate"] = (long int) since;

  json alerts = sendCommand("alertlist", options);

  for (auto& [i, alert] : alerts.items()) ret->push_back(BlueIrisAlert(alert));

  return ret;
}

bool MonitorServiceBlueIris::isAvailable() {
  MonitorServiceBase::isAvailable();

  try {
    json status = sendCommand("status");

    // TODO: Test the disk free
    for( const auto& disk : status["disks"] ) {
      cout << "Disk Free %: " << (float(disk["free"]) / float(disk["total"]) * 100) << endl;
    }

    // TODO: Test the warnings 
    unsigned int warnings = stoi(status["warnings"].get<string>());
    cout << "Warnings: " << warnings << endl;
    
    // TODO: Test the uptime
    cout << "Uptime: " << status["uptime"] << endl;

    // And now let's capture any interesting pictures we can include in our 
    // report:
    time_t now = time(nullptr);
    auto alerts = getAlerts(relative_time_from(now, capture_from), capture_camera);
    time_t alerts_upto = relative_time_from(now, capture_to);

    alerts->erase( remove_if( alerts->begin(), alerts->end(),
      [alerts_upto](const BlueIrisAlert & a) { return a.date > alerts_upto; }),
      alerts->end());

    map<string,string> get_header;
    get_header["Cookie"] = fmt::format("session={}", session);
    get_header["Accept"] = "image/webp,image/apng,image/*,*/*;q=0.8";

    for (auto& alert : *alerts) {
      string tmpbase = tmpnam(nullptr);

      // Jpg:
      auto [code, body] = client->get(alert.pathThumb(), get_header);
      // TODO: Test the code == 200
      ofstream out(fmt::format("{}.jpg", tmpnam(nullptr)));
      out << body;
      out.close();

      // WebP:
      tie(code, body) = client->get(alert.pathClip(), get_header);
      // TODO: Test the code == 200
      ofstream outb(fmt::format("{}-smaller.jpg", tmpnam(nullptr)));
      outb << body;
      outb.close();

      // TODO: Didn't we have animations?

      cout << "Alert: " << alert.dateAsString("%Y-%m-%d %H:%M") << " " << alert.camera << " "
        << " Received: " << tmpbase << endl; 
    }

  } catch(BlueIrisException& e) {
    // TODO
    cout << "BlueIrisException :" << e.what() << std::endl;
    return resultFail("BlueIrisException {}", e.what());
  } 

  return true;
}
