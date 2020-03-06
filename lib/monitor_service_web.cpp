#include "monitor_service_web.h"

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

  isHttps = false;
  port = 0;
  path = "/";
  status_equals = Poco::Net::HTTPResponse::HTTP_OK;

  setParameters({
    {"ensure_match",  [&](string v) { ensure_match = v;}},
    {"status_equals", [&](string v) { status_equals = stoi(v);} },
    {"path",          [&](string v) { path = v;} },
    {"port",          [&](string v) { port = stoi(v);} },
    {"proto",         [&](string v) { 
      if (v == "http")
        isHttps = false;
      else if (v == "https")
        isHttps = true;
      else
        throw invalid_argument(fmt::format("Unrecognized web proto \"{}\".", v));
    } }
  });

  if (!port) port = (isHttps) ? 443 : 80;

  client = make_unique<WebClient>(address, port, isHttps);
}

RESULT_TUPLE MonitorServiceWeb::fetchResults() {
  auto [errors, results] = MonitorServiceBase::fetchResults();

  try {
    auto [status_code, response_content] = client->get(path);

    (*results)["response_status"] = to_string(status_code);
    (*results)["response_content"] = response_content;

    if (status_code == status_equals) {
      if (!ensure_match.empty()) {
        auto re = regex(ensure_match);

        auto match_count = distance(
          sregex_iterator( response_content.begin(), response_content.end(), re), 
          sregex_iterator());

        (*results)["response_match_count"] = to_string(match_count);

        if ( match_count == 0 )
          err(errors, "Unable to find {} in content response", ensure_match);
      }
    }
    else
      err(errors,"Server status code {} did not match the expected {} status code.", 
        status_code, status_equals);
  } catch(const exception& e) { err(errors, "\"{}\" exception", e.what()); }

	return make_tuple(errors, results);
}
