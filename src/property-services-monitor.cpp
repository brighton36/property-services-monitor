#include "monitor_job.h"
#include "yaml-cpp/yaml.h"

using namespace std;

// TODO: shared_ptr? 
MonitorServiceFactory::map_type * MonitorServiceFactory::map = NULL;

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cerr << "Usage: " << argv[0] << " config.yml" << endl;
		return 1;
	}

  shared_ptr<MonitorJob> job;

  try {
    job = make_shared<MonitorJob>(string(argv[1]));
    job->printtest();
  } catch(const YAML::Exception& e) {
    cerr << "YAML Exception: " << e.what() << endl;
    return 1;
  } catch(const invalid_argument& e) {
    cerr << "invalid_argument Encountered: " << e.what() << endl;
    return 1;
  } catch(const exception& e) {
    cerr << "General Exception Encountered: " << e.what() << endl;
    return 1;
  }

  return 0;
}
