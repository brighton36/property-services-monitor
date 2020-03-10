#include "monitor_service_blueiris.h"

#include <ctime>

#include "Poco/MD5Engine.h"
#include "Poco/DigestStream.h"

#include "GraphicsMagick/Magick++.h"

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
  min_percent_free = 5;
  max_warnings = 0;
  min_uptime = 24 * 60 * 60;

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
    {"capture_from",    [&](string v) { capture_from = v;}},
    {"capture_to",      [&](string v) { capture_to = v;}},
    {"capture_camera",  [&](string v) { capture_camera = v;}},

    {"max_warnings",     [&](string v) { max_warnings = stoi(v); }},
    {"min_uptime",       [&](string v) { min_uptime = duration_to_seconds(v); }},
    {"min_percent_free", [&](string v) { 
      min_percent_free = percent_string_to_float(v); } },
  } );

  if (username.empty()) throw invalid_argument("Missing required field \"username\".");
  if (password.empty()) throw invalid_argument("Missing required field \"password\".");
 
  if (!port) port = (isSSL) ? 443 : 81;

  client = make_unique<WebClient>(address, port, isSSL);

  Magick::InitializeMagick(NULL);
}

MonitorServiceBlueIris::~MonitorServiceBlueIris() {
  // TODO:
  //if (!tmp_dir.empty()) filesystem::remove_all(tmp_dir);
}

string MonitorServiceBlueIris::createTempDirectory() {
  string ret = fmt::format("{}/psm_bi.XXXXXX", string(filesystem::temp_directory_path()));

  char * mkd_cstr = new char[ret.length()+1];
  strcpy(mkd_cstr, ret.c_str());

  if ( mkdtemp(mkd_cstr) == NULL )
    throw runtime_error("Error creating temp directory for blue iris downloads");

  ret = string(mkd_cstr);

  delete mkd_cstr;

  return ret;
}

unsigned int MonitorServiceBlueIris::uptime_to_seconds(string input) {
  smatch matches;
  auto match_uptime = regex("([\\d]+)\\:([\\d]+)\\:([\\d]+)\\:([\\d]+)");

  return (regex_search(input, matches, match_uptime)) ?
    ( stoi(matches[4])+stoi(matches[3])*60+stoi(matches[2])*60*60+
      stoi(matches[1])*60*60*24 ) : 0;
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
      throw BlueIrisException("{} with \"{}\" in response to authentication request .", 
        json_response["result"].get<string>(), json_response["data"]["reason"].get<string>());

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
    throw BlueIrisException("{} with \"{}\" in response to {} command.", 
      json_response["result"].get<string>(), json_response["data"]["reason"].get<string>(), command);

  return json_response["data"];
}

shared_ptr<vector<BlueIrisAlert>> 
MonitorServiceBlueIris::getAlertsCommand(time_t since = 0, string camera = "Index") {
  auto ret = make_shared<vector<BlueIrisAlert>>();

  json options = {{"camera", camera}};

  if (since > 0) options["startdate"] = (long int) since;

  json alerts = sendCommand("alertlist", options);

  for (auto& [i, alert] : alerts.items()) 
    ret->push_back(BlueIrisAlert(alert));

  return ret;
}

json MonitorServiceBlueIris::fetchImage(BlueIrisAlert &alert, string tmp_dir) { 
  map<string,string> get_header;
  get_header["Cookie"] = fmt::format("session={}", session);
  get_header["Accept"] = "image/webp,image/apng,image/*,*/*;q=0.8";

  string clip_source = alert.pathThumb();

  // TODO: Pull from config : time intervals of 3125... is there a ceil on this, per video?
  auto [code, body] = client->get(clip_source, get_header);

  if (code != 200) throw BlueIrisException(
    "Error code {} when attempting to download {}.", code, clip_source);

  Magick::Blob blob(static_cast<const void *>(body.c_str()), body.length());
  Magick::Image image;
  image.read(blob);

  unsigned int width = image.columns();
  unsigned int height = image.rows();

  // TODO: Pull this from the config
  width = width / 2;
  height = height / 2;
  image.resize(Magick::Geometry(width, height));

  string path_dest = fmt::format("{}/{}_{}.jpg", tmp_dir, alert.camera, alert.date);

  cout << fmt::format("{}: {}x{}", path_dest, width, height) << endl;

  image.write(path_dest);

  return { {"src", path_dest}, {"width", width}, {"height", height},
    {"alt", fmt::format("{} {}", alert.camera, alert.dateAsString("%F %r")) } };
}

json MonitorServiceBlueIris::fetchAlertImages() {
  auto ret = json::array();

  // And now let's capture any interesting pictures we can include in our 
  // report:
  time_t now = time(nullptr);
  auto alerts = getAlertsCommand(relative_time_from(now, capture_from), capture_camera);
  time_t alerts_upto = relative_time_from(now, capture_to);

  alerts->erase( remove_if( alerts->begin(), alerts->end(),
    [alerts_upto](const BlueIrisAlert & a) { return a.date > alerts_upto; }),
    alerts->end());

  // Create a temporary directory to work with for our downloads:
  if (tmp_dir.empty()) tmp_dir = createTempDirectory();

  for (auto& alert : *alerts)
    ret.push_back(fetchImage(alert, tmp_dir));

  return ret;
}

RESULT_TUPLE MonitorServiceBlueIris::fetchResults() {
  auto [errors, results] = MonitorServiceBase::fetchResults();

  try {
    json status = sendCommand("status");

    for( const auto& disk : status["disks"] ) {
      float percent_free = disk["free"].get<float>() / disk["total"].get<float>() * 100;

      if (percent_free < min_percent_free)
        err(errors, "Less than {0:.1f}% remaining on disk.", min_percent_free);
    }

    unsigned int warnings = stoi(status["warnings"].get<string>());
    if (warnings > max_warnings)
      err(errors, "System log returned {} warnings", warnings);
    
    if (uptime_to_seconds(status["uptime"]) < min_uptime)
      err(errors, 
        "The system uptime is low, at \"{}\". Seems as if the power was recently cycled", 
        status["uptime"].get<string>());

    (*results)["images"] = fetchAlertImages();
    (*results)["status"] = status;

  } catch(const exception& e) { 
    cout << "BlueIrisException :" << e.what() << std::endl;
    err(errors, "Encountered {}", e.what()); 
  }

  return make_tuple(errors, results);
}
