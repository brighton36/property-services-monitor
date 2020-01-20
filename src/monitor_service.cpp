#include <iostream>
#include <vector>

#include "monitor_job.h"

#include "Poco/Net/ICMPClient.h"
#include "Poco/Net/IPAddress.h"
#include "Poco/Net/ICMPEventArgs.h"
#include "Poco/Delegate.h"

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

using Poco::Net::ICMPClient;
using Poco::Net::IPAddress;
using Poco::Net::ICMPEventArgs;

MonitorServicePing::MonitorServicePing(string address) 
  : MonitorServiceBase(address, "ping") { 
}

MonitorServicePing::~MonitorServicePing() {
  cout << "~MonitorServicePing()" << endl;
}

#ifdef __GNUC__
#define UNUSED __attribute((unused))
#else
#define UNUSED
#endif

class PingExample {
	public:
	PingExample(): 
		_icmpClient(IPAddress::IPv4) {
    _icmpClient.pingBegin += Poco::delegate(this, &PingExample::onBegin);
    _icmpClient.pingReply += Poco::delegate(this, &PingExample::onReply);
    _icmpClient.pingError += Poco::delegate(this, &PingExample::onError);
    _icmpClient.pingEnd   += Poco::delegate(this, &PingExample::onEnd);
	}

	void start_ping(const std::string& host) {
		_icmpClient.ping(host);
	}


	void onBegin(const void* pSender UNUSED, ICMPEventArgs& args) {
		std::cout << "Pinging " << args.hostName() << " [" 
		<< args.hostAddress() << "] with " << args.dataSize() << " bytes of data:" 
		<< std::endl << "---------------------------------------------" << std::endl;
	}

	void onReply(const void* pSender UNUSED, ICMPEventArgs& args) {
		std::cout << "Reply from " << args.hostAddress()
			<< " bytes=" << args.dataSize() 
			<< " time=" << args.replyTime() << "ms"
			<< " TTL=" << args.ttl();
	}

	void onError(const void* pSender UNUSED, ICMPEventArgs& args) {
		std::cout << args.error();
	}

	void onEnd(const void* pSender UNUSED, ICMPEventArgs& args) {
		std::cout << std::endl << "--- Ping statistics for " << args.hostName() << " ---"
		<< std::endl << "Packets: Sent=" << args.sent() << ", Received=" << args.received()
		<< " Lost=" << args.repetitions() - args.received() << " (" << 100.0 - args.percent() << "% loss),"
		<< std::endl << "Approximate round trip times in milliseconds: " << std::endl
		<< "Minimum=" << args.minRTT() << "ms, Maximum=" << args.maxRTT()  
		<< "ms, Average=" << args.avgRTT() << "ms" 
		<< std::endl << "------------------------------------------";
	}

	private:
		ICMPClient  _icmpClient;
};

bool MonitorServicePing::IsAvailable() {
  cout << "Service Ping is available " << endl;
	//PingExample p;
	//p.start_ping("localhost");
  return false;
}
