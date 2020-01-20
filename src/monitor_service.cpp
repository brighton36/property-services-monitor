#include "monitor_job.h"

using namespace std;

MonitorServiceBase::MonitorServiceBase(string address, string type) {
  this->address = address;
  this->type = type;
}

bool MonitorServiceBase::IsAvailable() {
  cout << "TODO: in base, throw error?" << endl;
  return false;}

MonitorServiceBase::~MonitorServiceBase() {
  cout << "~MonitorServiceBase()" << endl;
}

MonitorServiceWeb::MonitorServiceWeb(string address) 
  : MonitorServiceBase(address, "web") { }

MonitorServiceWeb::~MonitorServiceWeb() {
  cout << "~MonitorServiceWeb()" << endl;
}

