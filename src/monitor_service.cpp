#include "monitor_job.h"

using namespace std;

MonitorServiceBase::MonitorServiceBase(string type, string address, PTR_UMAP_STR params) {
  this->type = type;
  this->address = address;
  this->params = params;
}

bool MonitorServiceBase::IsAvailable() {
  return false;
}

PTR_UMAP_STR MonitorServiceBase::Results() {
  // TODO: The template will likely want this...
  PTR_UMAP_STR results;

  return results;
}

ServiceRegister<MonitorServiceWeb> MonitorServiceWeb::reg("web");

MonitorServiceWeb::MonitorServiceWeb(string address, PTR_UMAP_STR params) 
  : MonitorServiceBase("web", address, params) {
}

