#include "monitor_service_blueiris.h"

#include <ctime>

#include "Poco/MD5Engine.h"
#include "Poco/DigestStream.h"

using namespace std;
using namespace nlohmann;

ServiceRegister<MonitorServiceBlueIris> MonitorServiceBlueIris::reg("blueiris");

std::string MonitorServiceBlueIris::Help() { 
  return 
    " * username         (required) A username for use in logging into the blue iris web service.\n"
    " * password         (required) A password for use in logging into the blue iris web service.\n"
    " * proto            (optional) Either \"http\" or \"https\". Defaults to \"http\".\n"
    " * port             (optional) The port number of the http service. Defaults to 80 (http) or\n"
    "                             443 (https), depending on the proto value.\n"
    " * capture_camera   (optional) Which camera(s) to capture alert images from, for inclusion\n"
    "                               in our report. Defaults to \"Index\" (Include All cameras).\n"
    " * capture_from     (optional) A human readable DATETIME, indicating what time and day will\n"
    "                               start our alert image capturing. This is used to download\n"
    "                               and attach images to our report. See the notes on DATETIME\n"
    "                               formatting further below. Defaults to \"11:00P the day before\".\n"
    " * capture_to       (optional) A human readable DATETIME, indicating what time and day will\n"
    "                               end our alert image capturing. This is used to download\n"
    "                               and attach images to our report. See the notes on DATETIME\n"
    "                               formatting further below. Defaults to \"Today at 4:00A\".\n"
    " * ignore_warnings  (optional) Set to \"1\" to simply ignore warnings. Defaults to 0.\n"
    " * max_warnings     (optional) Warnings are returned by the \"status\" command, to the \n"
    "                               blue iris server. This is the count threshold, above which\n"
    "                               we trigger failure. Defaults to 0.\n"
    " * min_uptime       (optional) The minimum acceptable system uptime for this server. This\n"
    "                               parameter is expected to be provided in the DURATION format.\n"
    "                               For more details on the DURATION format, see below. Defaults\n"
    "                               to \"1 day\".\n"
    " * min_percent_free (optional) The minimum amount of space required on the system hard disk(s)\n"
    "                               in order to pass this check. This test is applied to every drive\n"
    "                               installed in the system. Defaults to \"5%\".\n";
}

MonitorServiceBlueIris::MonitorServiceBlueIris(string address, PTR_MAP_STR_STR params) 
  : MonitorServiceBase("blueiris", address, params) { 
  unsigned int port = 0;
  bool isSSL = false;
  session = string();
  capture_from = "11:00P the day before";
  capture_to = "Today at 4:00A";
  capture_camera = "Index";
  min_percent_free = 5;
  max_warnings = 0;
  ignore_warnings = 0;
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
    {"capture_camera",  [&](string v)  { capture_camera = v;}},
    {"capture_from",    [&](string v)  { capture_from = v;}},
    {"capture_to",      [&](string v)  { capture_to = v;}},

    {"max_warnings",     [&](string v) { max_warnings = stoi(v); }},
    {"ignore_warnings",     [&](string v) { ignore_warnings = stoi(v); }},
    {"min_uptime",       [&](string v) { min_uptime = duration_to_seconds(v); }},
    {"min_percent_free", [&](string v) { 
      min_percent_free = percent_string_to_float(v); } },
  } );

  if (username.empty()) throw invalid_argument("Missing required field \"username\".");
  if (password.empty()) throw invalid_argument("Missing required field \"password\".");
 
  if (!port) port = (isSSL) ? 443 : 81;

  client = make_unique<WebClient>(address, port, isSSL);
}

MonitorServiceBlueIris::~MonitorServiceBlueIris() {
	//if (!tmp_dir.empty()) filesystem::remove_all(tmp_dir);
}

string MonitorServiceBlueIris::createTempDirectory() {
  string ret = fmt::format("{}/psm_bi.XXXXXX", string(filesystem::temp_directory_path()));

  char * mkd_cstr = new char[ret.length()+1];
  strcpy(mkd_cstr, ret.c_str());

  if ( mkdtemp(mkd_cstr) == NULL )
    throw runtime_error("Error creating temp directory for blue iris downloads");

  ret = string(mkd_cstr);

  destroy_at(mkd_cstr);

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

// I grabbed and modified this from : http://www.wischik.com/lu/programmer/get-image-size.html
tuple<unsigned int, unsigned int> 
MonitorServiceBlueIris::imageDimensions(const char *image, unsigned int length) {
  if (length<24) throw BlueIrisException("Invalid jpeg file.");

  unsigned char buf[24]; 
  if (!memcpy(buf, image, 24))
    throw BlueIrisException("Unable to process jpeg file.");

  // For JPEGs, we need to read the first 12 bytes of each chunk.
  // We'll read those 12 bytes at buf+2...buf+14, i.e. overwriting the existing buf.
  if (buf[0]==0xFF && buf[1]==0xD8 && buf[2]==0xFF && buf[3]==0xE0 && 
		buf[6]=='J' && buf[7]=='F' && buf[8]=='I' && buf[9]=='F') { 
		unsigned int pos=2;
    while (buf[2]==0xFF) { 
			if (buf[3]==0xC0 || buf[3]==0xC1 || buf[3]==0xC2 || buf[3]==0xC3 || 
				buf[3]==0xC9 || buf[3]==0xCA || buf[3]==0xCB) 
				break;

      pos += 2+(buf[4]<<8)+buf[5];

      if (pos+12>length) break;

      if(!memcpy(buf+2, image+pos, 12))
        throw BlueIrisException("Unable to process jpeg file.");
    }
  }

  // JPEG: (first two bytes of buf are first two bytes of the jpeg file; rest of 
  // buf is the DCT frame
  if (buf[0]==0xFF && buf[1]==0xD8 && buf[2]==0xFF)
    return make_tuple((buf[9]<<8) + buf[10], (buf[7]<<8) + buf[8]);

  throw BlueIrisException("Unable to ascertain dimensions of jpg file.");
}

json MonitorServiceBlueIris::fetchImage(BlueIrisAlert &alert, string tmp_dir) { 
  string path_dest = fmt::format("{}/{}_{}.jpg", tmp_dir, alert.camera, alert.date);

  auto [code, body] = client->get(alert.pathThumb(), {
    {"Cookie", fmt::format("session={}", session)},
    {"Accept", "image/webp,image/apng,image/*,*/*;q=0.8"} });

  if (code != 200) throw BlueIrisException(
    "Error code {} when attempting to download {}.", code, alert.pathThumb());

    auto [width, height] = imageDimensions(body.c_str(), body.length());

    ofstream outb(path_dest);
    outb << body;
    outb.close();

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

  reverse(alerts->begin(),alerts->end());

  for (auto& alert : *alerts) ret.push_back(fetchImage(alert, tmp_dir));

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

    if (!ignore_warnings) {
      unsigned int warnings = stoi(status["warnings"].get<string>());
      if (warnings > max_warnings)
        err(errors, "System log returned {} warnings", warnings);
    }
    
    if (uptime_to_seconds(status["uptime"]) < min_uptime)
      err(errors, 
        "The system uptime is low, at \"{}\". Seems as if the power was recently cycled", 
        status["uptime"].get<string>());

    (*results)["images"] = fetchAlertImages();
    (*results)["status"] = status;

  } catch(const exception& e) { 
    err(errors, "Encountered {}", e.what()); 
  }

  return make_tuple(errors, results);
}
