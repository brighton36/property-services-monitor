#include "monitor_service_blueiris.h"

#include <ctime>

#include "Poco/MD5Engine.h"
#include "Poco/DigestStream.h"

using namespace std;
using namespace nlohmann;

ServiceRegister<MonitorServiceBlueIris> MonitorServiceBlueIris::reg("blueiris");

BlueIrisAlert::BlueIrisAlert(nlohmann::json from) {
	camera    = from["camera"];
	clip      = from["clip"];
	color     = from["color"];
	filesize  = from["filesize"];
	flags     = from["flags"];
  newalerts = from.value("newalerts", 0);
	offset    = from["offset"];
	path      = from["path"];
	res       = from["res"];
	zones     = from["zones"];

  // NOTE: I'm ignoring zones here. Which, seems to work as expected as long as
  // "we" are in the same zone as the server we're querying. 
  date = static_cast<time_t>(from["date"]);
}

std::string BlueIrisAlert::dateAsString(std::string fmt) {
  char strf_out[80];

  strftime(strf_out,80,fmt.c_str(),localtime(&date));

  return std::string(strf_out);
}

std::string BlueIrisAlert::pathThumb() { 
  return fmt::format("/thumbs/{}", path);
}

std::string BlueIrisAlert::pathClip() { 
  return fmt::format("/file/clips/{}?time=0&cache=1&h=240", clip);
}
std::string MonitorServiceBlueIris::Help() { 
  return "TODO";
}

MonitorServiceBlueIris::MonitorServiceBlueIris(string address, PTR_MAP_STR_STR params) 
  : MonitorServiceBase("blueiris", address, params) { 
  // TODO : Params 
  
  unsigned int port = 81; // TODO: wot?
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

  // TODO: Username and pass are required. Fail if omitted
  //  throw invalid_argument(fmt::format("Unrecognized web proto \"{}\".", v));
  //
  if (!port) port = (isSSL) ? 443 : 80;

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

    cout << "Status:" << status.dump() << endl;

    // TODO: If params are configured, test that the disks amount free is above the 
    // threshold. Maybe check the warnings is under a threshold and that uptime is 
    // over ... a threshold

    // And now let's scoop up any interesting pictures we can include in our 
    // report:
    time_t now = time(nullptr);
    struct tm since = *localtime(&now);
    since.tm_hour -= 1; // TODO: Figure out how to pull these...
    
    auto alerts = getAlerts(mktime(&since));

    map<string,string> get_header;
    get_header["Cookie"] = fmt::format("session={}", session);
    get_header["Accept"] = "image/webp,image/apng,image/*,*/*;q=0.8";

    for (auto& alert : *alerts) {
      cout << "Camera Alert: " << alert.camera << " @ " << alert.dateAsString("%Y-%m-%d %H:%M") << endl;
      auto [code, body] = client->get(alert.pathThumb(), get_header);
      cout << "   - Received:" << code << "Body: " << body.length() << endl; 
    }

  } catch(BlueIrisException& e) {
    // TODO
    cout << "BlueIrisException :" << e.what() << std::endl;
    return resultFail("BlueIrisException {}", e.what());
  } 

  return true;
}
