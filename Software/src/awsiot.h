#include <iostream>
#include <aws/crt/Api.h>
#include <aws/crt/StlAllocator.h>
#include <aws/crt/auth/Credentials.h>
#include <aws/crt/io/TlsOptions.h>

#include <aws/iot/MqttClient.h>

#include <algorithm>
#include <aws/crt/UUID.h>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <typeinfo>
#include <string>

using namespace Aws::Crt;

class AwsIot{
public:
    AwsIot();
    void iotReadJson();
    void checkTimestamp();
    
 //   std::promise<bool> connectionCompletedPromise;
 //   std::promise<void> connectionClosedPromise;
 //   auto connection;
    
    int iotSampleRateIndex;
    double iotLowerBandwidth;
    double iotUpperBandwidth;
    double iotDspCutoffFreq;
    bool stop;    // std::atomic<bool> stop;
  //  std::string message;
    std::string current_timestamp; 
    std::string new_timestamp;
    std::mutex mtx;
    std::condition_variable start_recording; 

};
	
