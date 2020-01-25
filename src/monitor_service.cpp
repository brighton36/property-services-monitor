#include "monitor_job.h"

using namespace std;

MonitorServiceBase::MonitorServiceBase(string type, string address, PTR_UMAP_STR params) {
  this->type = type;
  this->address = address;
  this->params = params;
  this->results = make_shared<unordered_map<string, string>>();
}

bool MonitorServiceBase::IsAvailable() { 
  this->results->clear();
  // TODO: Do we want to set each param here?

  return false;
}

