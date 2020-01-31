#include "monitor_job.h"
#include "yaml-cpp/yaml.h"

#include <filesystem>

using namespace std;

#define MISSING_FIELD "Missing \"{}\" field."
#define MISSING_HOST_FIELD "Missing {} for host \"{}\"."
#define CANT_READ "Unable to open file {}."

bool MonitorJob::PathIsReadable(string path) {
	filesystem::path p(path);

	error_code ec;
	auto perms = filesystem::status(p, ec).permissions();

	return ( (ec.value() == 0) && (
    (perms & filesystem::perms::owner_read) != filesystem::perms::none &&
    (perms & filesystem::perms::group_read) != filesystem::perms::none &&
    (perms & filesystem::perms::others_read) != filesystem::perms::none ) );
}

MonitorJob::MonitorJob(string path) {
  config_path = path;

  const auto config = YAML::LoadFile(config_path);

  if (!config["to"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "to"));
  if (!config["from"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "from"));
  if (!config["subject"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "subject"));
  if (!config["hosts"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "hosts"));
  if (!config["template_html"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "template_html"));
  if (!config["template_plain"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "template_plain"));

  to = config["to"].as<string>();
  from = config["from"].as<string>();
  subject = config["subject"].as<string>();
  template_html_path = config["template_html"].as<string>();
  template_plain_path = config["template_plain"].as<string>();

  if (!PathIsReadable(template_html_path)) 
    throw invalid_argument(fmt::format(CANT_READ, template_html_path));
  if (!PathIsReadable(template_plain_path)) 
    throw invalid_argument(fmt::format(CANT_READ, template_plain_path));

	this->smtp_params = make_shared<unordered_map<string, string>>();
  
  if ( (!config["smtp"]) || (!config["smtp"].IsMap()))
    throw invalid_argument(fmt::format("Smtp settings missing", "smtp"));

  for(auto it=config["smtp"].begin();it!=config["smtp"].end();++it) {
    const auto param = it->first.as<std::string>();
    const auto value = it->second.as<std::string>();
    
    this->smtp_params->insert(make_pair(param, value));
  }

  if (config["hosts"].size() < 1) throw invalid_argument("No Hosts to monitor."); 
  
  for (const auto config_host: config["hosts"]) {
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
      
      PTR_UMAP_STR params = make_shared<unordered_map<string, string>>();

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

nlohmann::json MonitorJob::ToJson() {
  nlohmann::json ret;

  // TODO: These should go in the yaml...
  ret["text_color"] = "#0D1B1E";
  ret["margin_color"] = "#C3DBC5";
  ret["border_color"] = "#7798AB";
  ret["body_color"] = "#E8DCB9";
  ret["alert_color"] = "#F2CEE6";

  // TODO: I think most/all of this should go into the smtp object
  ret["to"] = this->to;
  ret["from"] = this->from;
  ret["subject"] = this->subject;
  ret["hosts"] = nlohmann::json::array();
  ret["template_html"] = this->template_html_path;
  ret["has_failures"] = false;
  // /TODO

  for (const auto host: this->hosts) {
    auto json_host = nlohmann::json::object();
    json_host["label"] = host->label;
    json_host["description"] = host->description;
    json_host["address"] = host->address;
    json_host["services"] = nlohmann::json::array();

    for(const auto service: host->services) {
      auto json_service = nlohmann::json::object();
      bool is_up = service->IsAvailable();

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

MonitorHost::MonitorHost(string label, string description, string address, 
    vector<shared_ptr<MonitorServiceBase>> services) {
  this->label = label;
  this->description = description;
  this->address = address;
  this->services = services;
}

