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

time_t relative_time_from(time_t starting_at, string adjustment) {
	smatch matches;

	time_t ret = starting_at;

  auto match_hh_mm = regex("([\\d]{1,2})\\:([\\d]{2})(?:[ ]*([p|P])|)");
  auto match_day =  regex(
    "(?:(yesterday)|([\\d]+|the)[ ]*day[s]?[ ]*|day[s]?[ ]*(?:prior|ago|before|back))", 
    regex_constants::icase
  );
  auto match_hour =  regex(
    "(?:([\\d]+|the)[ ]*hour[s]?[ ]*|hour[s]?[ ]*(?:prior|ago|before|back))", 
    regex_constants::icase
  );
  auto match_min =  regex(
    "(?:([\\d]+|the)[ ]*(?:min|minute)[s]?[ ]*|(?:min|minute)[s]?[ ]*(?:prior|ago|before|back))", 
    regex_constants::icase
  );

  // These are the spin-backward instructions
  if (regex_search(adjustment, matches, match_day)) {
    unsigned int days_back = 1;

    if ((!string(matches[2]).empty()) && regex_match(string(matches[2]), regex("^[\\d]+$")))
      days_back = stoi(matches[2]);

    ret -= (days_back * 24 * 60 * 60);
  }

  if (regex_search(adjustment, matches, match_hour)) {
    ret -= ( (matches[1] == "the") ? 1 : stoi(matches[1]) ) * (60 * 60);
  } 

  if (regex_search(adjustment, matches, match_min)) {
    ret -= ( (matches[1] == "the") ? 1 : stoi(matches[1]) ) * 60;
  }

  if(regex_search(adjustment, matches, match_hh_mm)) {
    unsigned int set_hour = stoi(matches[1]);
    unsigned int set_minute = stoi(matches[2]);

    // Check for a PM indication:
    if ( !string(matches[3]).empty() && (set_hour < 12)) set_hour += 12;

    struct tm *tm_ret = localtime(&ret);
    tm_ret->tm_hour = set_hour;
    tm_ret->tm_min = set_minute;

    ret = mktime(tm_ret);
  }

  return ret;
}
