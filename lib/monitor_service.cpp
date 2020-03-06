#include "property-services-monitor.h"

using namespace std;

MonitorServiceBase::MonitorServiceBase(string t, string a, PTR_MAP_STR_STR p) {
  type = t;
  address = a;
  params = p;
}

RESULT_TUPLE MonitorServiceBase::fetchResults() {
  auto results = make_shared<map<string, string>>();
  auto errors = make_shared<vector<string>>();

	return make_tuple(errors, results);
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
