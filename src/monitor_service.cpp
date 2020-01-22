#include "monitor_job.h"

using namespace std;

MonitorServiceBase::MonitorServiceBase(string type, string address, unordered_map<string, string> params) {
  this->type = type;
  this->address = address;
  this->params = params;
  cout << "   MonitorServiceBase():" << address << " " << type << endl;
}

bool MonitorServiceBase::IsAvailable() {
  cout << "TODO: in base, throw error?" << endl;
  return false;}

MonitorServiceBase::~MonitorServiceBase() {
  cout << "~MonitorServiceBase()" << endl;
}

ServiceRegister<MonitorServiceWeb> MonitorServiceWeb::reg("web");

MonitorServiceWeb::MonitorServiceWeb(string address, unordered_map<string, string> params) 
  : MonitorServiceBase("web", address, params) {
  cout << "   MonitorServiceWeb() :" << address << endl;
  for( const auto& n : this->params ) {
    std::cout << "Param Key:[" << n.first << "] Value:[" << n.second << "]\n";
  }

  }


MonitorServiceWeb::~MonitorServiceWeb() {
  cout << "~MonitorServiceWeb()" << endl;
}

