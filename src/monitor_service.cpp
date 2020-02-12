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

void MonitorServiceBase::setParameters(map<string,function<void(string)>> assigns) {
  for( const auto& p : *params ) {
  	auto it = assigns.find(p.first);
		if (it != assigns.end()) 
      assigns[p.first](p.second);
    else
      throw invalid_argument(fmt::format("Unrecognized {} parameter \"{}\".", 
        type, p.first));
	}

  return; 
}
