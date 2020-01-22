#include "monitor_job.h"

using namespace std;

MonitorServiceBase::MonitorServiceBase(string type, string address, PTR_UMAP_STR params) {
  this->type = type;
  this->address = address;
  this->params = params;

  for( const auto& n : *params ) {
    fmt::print("{} : Param Key:[{}] Value:[{}]\n", this->address, n.first, n.second);
  }

}

bool MonitorServiceBase::IsAvailable() {
  cout << "TODO: in base, throw error?" << endl;
  return false;
}

ServiceRegister<MonitorServiceWeb> MonitorServiceWeb::reg("web");

MonitorServiceWeb::MonitorServiceWeb(string address, PTR_UMAP_STR params) 
  : MonitorServiceBase("web", address, params) {

  // TODO:
}

