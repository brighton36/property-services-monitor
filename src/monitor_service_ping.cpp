#include "monitor_job.h"

#include "Poco/Net/ICMPClient.h"

using namespace std;

ServiceRegister<MonitorServicePing> MonitorServicePing::reg("ping");

MonitorServicePing::MonitorServicePing(string address, unordered_map<string, string> params) 
  : MonitorServiceBase("ping", address, params) { 
  cout << "   MonitorServicePing():" << address << endl;
  for( const auto& n : this->params ) {
    std::cout << "Param Key:[" << n.first << "] Value:[" << n.second << "]\n";
  }

}

MonitorServicePing::~MonitorServicePing() {
  cout << "~MonitorServicePing()" << endl;
}

unordered_map<string, string> MonitorServicePing::Results() {
  unordered_map<string, string> results;

  return results;
}

bool MonitorServicePing::IsAvailable() {
  // TODO: Cache result?
  
  cout << "Ping :" ;

	try {
		const int attempts = 5;

		Poco::Net::SocketAddress socketAddress;
    socketAddress = Poco::Net::SocketAddress("127.0.0.1", "80");
    //socketAddress = Poco::Net::SocketAddress("192.168.1.127", "80");

		int successful_pings = Poco::Net::ICMPClient::ping(
      socketAddress, Poco::Net::IPAddress::IPv4, attempts);

    if (successful_pings == attempts)
      cout << "OK!" << endl;
    else
      cout << "FAIL" << endl;

	} catch(Poco::IOException& e) {
		cout << "Must run as root. TODO: Bubble it up " << endl;
	}



  return false;
}
