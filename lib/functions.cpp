#include <vector>
#include <algorithm> 
#include <filesystem>

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
