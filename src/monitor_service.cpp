#include "monitor_job.h"

using namespace std;

MonitorServiceBase::MonitorServiceBase(string type, string address, UMAP_STRING_STRING params) {
  this->type = type;
  this->address = address;
  this->params = params;
}

bool MonitorServiceBase::IsAvailable() {
  cout << "TODO: in base, throw error?" << endl;
  return false;
}

ServiceRegister<MonitorServiceWeb> MonitorServiceWeb::reg("web");

MonitorServiceWeb::MonitorServiceWeb(string address, UMAP_STRING_STRING params) 
  : MonitorServiceBase("web", address, params) {
  for( const auto& n : this->params ) {
    std::cout << "Param Key:[" << n.first << "] Value:[" << n.second << "]\n";
  }
}

