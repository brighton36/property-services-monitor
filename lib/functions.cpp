#include <vector>
#include <vector>
#include <algorithm> 
#include <filesystem>
#include <regex>

using namespace std;

bool has_any(vector<string> haystack, vector<string> needles) {
  return (find_if(haystack.begin(), haystack.end(), 
    [&needles] (string s) { 
      return (
        find_if(needles.begin(), needles.end(), [&s] (string n) { return n == s; }
      ) != needles.end());
    } 
  ) != haystack.end());
}

bool pathIsReadable(string path) {
  filesystem::path p(path);

  error_code ec;
  auto perms = filesystem::status(p, ec).permissions();

  return ( (ec.value() == 0) && (
    (perms & filesystem::perms::owner_read) != filesystem::perms::none &&
    (perms & filesystem::perms::group_read) != filesystem::perms::none &&
    (perms & filesystem::perms::others_read) != filesystem::perms::none ) );
}

unsigned int duration_to_seconds(string duration) {
  // TODO
  return 0;
}

unsigned int percent_string_to_float(string duration) {

  // TODO
  return 0;
}

time_t relative_time_from(time_t starting_at, string adjustment) {
	smatch matches;

  auto match_hh_mm = regex("([\\d]{1,2})\\:([\\d]{2})(?:[ ]*([p|P])|)");
  auto match_day = regex(
    "(?:yesterday|([\\d]+|the)[ ]*day[s]?[ ]*|day[s]?[ ]*(?:prior|ago|before|back))", 
    regex_constants::icase
  );
  auto match_hour = regex(
    "(?:([\\d]+|the)[ ]*hour[s]?[ ]*|hour[s]?[ ]*(?:prior|ago|before|back))", 
    regex_constants::icase
  );
  auto match_min = regex(
    "(?:([\\d]+|the)[ ]*(?:min|minute)[s]?[ ]*|(?:min|minute)[s]?[ ]*(?:prior|ago|before|back))", 
    regex_constants::icase
  );
  auto match_num = regex("^[\\d]+$");

  // These are the spin-backward instructions
  if (regex_search(adjustment, matches, match_day))
    starting_at -= 24 * 60 * 60 * (
      (regex_match(string(matches[1]), match_num)) ? stoi(matches[1]) : 1);

  if (regex_search(adjustment, matches, match_hour))
    starting_at -= 60 * 60 * (
      (regex_match(string(matches[1]), match_num)) ? stoi(matches[1]) : 1);

  if (regex_search(adjustment, matches, match_min))
    starting_at -= 60 * ( 
      (regex_match(string(matches[1]), match_num)) ? stoi(matches[1]) : 1);

  if(regex_search(adjustment, matches, match_hh_mm)) {
    unsigned int set_hour = stoi(matches[1]);
    unsigned int set_minute = stoi(matches[2]);

    // Check for a PM indication:
    if ( !string(matches[3]).empty() && (set_hour < 12)) set_hour += 12;

    struct tm *tm_ret = localtime(&starting_at);
    tm_ret->tm_hour = set_hour;
    tm_ret->tm_min = set_minute;

    starting_at = mktime(tm_ret);
  }

  return starting_at;
}
