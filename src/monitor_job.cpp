#include "property-services-monitor.h"
#include "yaml-cpp/yaml.h"

using namespace std;

#define MISSING_HOST_FIELD "Missing {} for host \"{}\"."

MonitorJob::MonitorJob(const YAML::Node config) {

  if (config.size() < 1) throw invalid_argument("No Hosts to monitor."); 
  
  for (const auto config_host: config) {
    if (!config_host["label"]) 
      throw invalid_argument("One or more hosts are missing a label.");

    const auto label = config_host["label"].as<string>();

    if (!config_host["address"]) 
      throw invalid_argument(fmt::format(MISSING_HOST_FIELD, "address", label));

    if (config_host["services"].size() < 1 || !config_host["services"].IsSequence()) 
      throw invalid_argument(fmt::format(MISSING_HOST_FIELD, "services", label));

    const auto description = (config_host["description"]) ? 
      config_host["description"].as<string>() : string();

    auto address = config_host["address"].as<string>();

    vector<shared_ptr<MonitorServiceBase>> services;

    for (const auto config_service: config_host["services"]) {
      string type;
      
      PTR_MAP_STR_STR params = make_shared<map<string, string>>();

      shared_ptr<MonitorServiceBase> service;

      if (config_service.IsMap()) {
        if (!config_service["type"]) 
          throw invalid_argument(
            fmt::format("Missing a service type under host \"{}\"", label)); 

        for(auto it=config_service.begin();it!=config_service.end();++it) {
          const auto param = it->first.as<std::string>();
          const auto value = it->second.as<std::string>();

          if (param == "type")
            type = value;
          else
            params->insert(make_pair(param, value));
        }
      } else {
        type = config_service.as<string>();
      }

      service = MonitorServiceFactory::createInstance(type, address, params);
      
      if (service == nullptr)
        throw invalid_argument(
          fmt::format("Invalid Service encountered for host \"{}\"", label)); 

      services.push_back(service);
    }

    hosts.push_back(make_shared<MonitorHost>( label, description, address, services ));
  }
} 

nlohmann::json MonitorJob::toJson() {
  nlohmann::json ret;

  ret["has_failures"] = false;
  ret["hosts"] = nlohmann::json::array();

  for (const auto host: hosts) {
    auto json_host = nlohmann::json::object();
    json_host["label"] = host->label;
    json_host["description"] = host->description;
    json_host["address"] = host->address;
    json_host["services"] = nlohmann::json::array();

    for(const auto service: host->services) {
      auto json_service = nlohmann::json::object();
      bool is_up = service->isAvailable();

      if (!is_up) ret["has_failures"] = true;

      json_service["type"] = service->type;
      json_service["is_up"] = is_up;
      json_service["results"] = nlohmann::json::object();

      for(auto it=service->results->begin();it!=service->results->end();++it)
        json_service["results"][it->first] = it->second;

      json_host["services"].push_back(json_service);
    }

    ret["hosts"].push_back(json_host);
  }

  return ret;
}

MonitorHost::MonitorHost(string l, string d, string a, vector<shared_ptr<MonitorServiceBase>> s) {
  label = l;
  description = d;
  address = a;
  services = s;
}

