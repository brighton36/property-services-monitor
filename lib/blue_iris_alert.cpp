#include "monitor_service_blueiris.h"

using namespace nlohmann;
using namespace std;

BlueIrisAlert::BlueIrisAlert(json from) {
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

string BlueIrisAlert::dateAsString(string fmt) {
  char strf_out[80];

  strftime(strf_out,80,fmt.c_str(),localtime(&date));

  return string(strf_out);
}

string BlueIrisAlert::pathThumb() { 
  return fmt::format("/thumbs/{}", path);
}

string BlueIrisAlert::pathClip() { 
  return fmt::format("/file/clips/{}?time=0&cache=1&h=240", clip);
}
