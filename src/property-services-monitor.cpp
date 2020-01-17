#include <iostream>
#include "yaml-cpp/yaml.h"
#include "monitor_job.h"

using namespace std;

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cerr << "Usage: " << argv[0] << " config.yml" << endl;
		return 1;
	}

  MonitorJob *job;

  try {
    job = new MonitorJob(argv[1]);
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

  delete job;

  return 0;
}
