#include "property-services-monitor.h"

using namespace std;

MonitorServiceBase::MonitorServiceBase(string t, string a, PTR_MAP_STR_STR p) {
  type = t;
  address = a;
  params = p;
  results = make_shared<map<string, string>>();
}

bool MonitorServiceBase::isAvailable() { 
  results->clear();
  return true;
}

bool MonitorServiceBase::resultAdd(string key, string value) { 
  results->emplace(key, value);
  return true;
}
