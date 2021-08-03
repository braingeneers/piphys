/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

#include "awsiot.h"
#include <jsoncpp/json/json.h>

using namespace Aws::Crt;

// constructor
AwsIot::AwsIot()
{
   //   message = "start";
      current_timestamp = "2020-09-22-20-26-00";
      std::cout << "current timestamp is " << current_timestamp << std::endl;
      stop = true;
}
  
void AwsIot::iotReadJson(){      
      
    /************************ Setup the Lib ****************************/
    /*
     * Do the global initialization for the API.
     */
    ApiHandle apiHandle;

    String endpoint = "ahp00abmtph4i-ats.iot.us-west-2.amazonaws.com";
    String certificatePath = "/home/pi/Desktop/raspintan-master/rhd_datastream/Certificates/7526fdcf8b-certificate.pem.crt";
    String keyPath = "/home/pi/Desktop/raspintan-master/rhd_datastream/Certificates/7526fdcf8b-private.pem.key";
    String caFile = "/home/pi/Desktop/raspintan-master/rhd_datastream/Certificates/AmazonRootCA1.pem";
    String topic("test/topic");
    String clientId(String("test-") + Aws::Crt::UUID().ToString());
    String signingRegion;
    String proxyHost;

    /********************** Setup an Mqtt Client ******************/
    /*
     * You need an event loop group to process IO events.
     * If you only have a few connections, 1 thread is ideal
     */
    Io::EventLoopGroup eventLoopGroup(1);
    if (!eventLoopGroup)
    {
        fprintf(
            stderr, "Event Loop Group Creation failed with error %s\n", ErrorDebugString(eventLoopGroup.LastError()));
        exit(-1);
    }
 
    Aws::Crt::Io::DefaultHostResolver defaultHostResolver(eventLoopGroup, 1, 5);
    Io::ClientBootstrap bootstrap(eventLoopGroup, defaultHostResolver);
 
    if (!bootstrap)
    {
        fprintf(stderr, "ClientBootstrap failed with error %s\n", ErrorDebugString(bootstrap.LastError()));
        exit(-1);
    }
 
    Aws::Crt::Io::TlsContext x509TlsCtx;
    Aws::Iot::MqttClientConnectionConfigBuilder builder;
 
    // check certificates
    if (!certificatePath.empty() && !keyPath.empty())
    {
        builder = Aws::Iot::MqttClientConnectionConfigBuilder(certificatePath.c_str(), keyPath.c_str());
    }
 
    if (!caFile.empty())
    {
        builder.WithCertificateAuthority(caFile.c_str());
    }
 
    builder.WithEndpoint(endpoint);
 
    auto clientConfig = builder.Build();
 
    if (!clientConfig)
    {
        fprintf(
            stderr,
            "Client Configuration initialization failed with error %s\n",
            ErrorDebugString(clientConfig.LastError()));
        exit(-1);
    }
 
    Aws::Iot::MqttClient mqttClient(bootstrap);
    /*
     * Since no exceptions are used, always check the bool operator
     * when an error could have occurred.
     */
    if (!mqttClient)
    {
        fprintf(stderr, "MQTT Client Creation failed with error %s\n", ErrorDebugString(mqttClient.LastError()));
        exit(-1);
    }
 
    /*
     * Now create a connection object. Note: This type is move only
     * and its underlying memory is managed by the client.
     */
    auto connection = mqttClient.NewConnection(clientConfig);
 
    if (!connection)
    {
        fprintf(stderr, "MQTT Connection Creation failed with error %s\n", ErrorDebugString(mqttClient.LastError()));
        exit(-1);
    }
 
    /*
     * In a real world application you probably don't want to enforce synchronous behavior
     * but this is a sample console application, so we'll just do that with a condition variable.
     */
    std::promise<bool> connectionCompletedPromise;
    std::promise<void> connectionClosedPromise;
 
    /*
     * This will execute when an mqtt connect has completed or failed.
     */
    auto onConnectionCompleted = [&](Mqtt::MqttConnection &, int errorCode, Mqtt::ReturnCode returnCode, bool) {
        if (errorCode)
        {
            fprintf(stdout, "Connection failed with error %s\n", ErrorDebugString(errorCode));
            connectionCompletedPromise.set_value(false);
        }
        else
        {
            if (returnCode != AWS_MQTT_CONNECT_ACCEPTED)
            {
                fprintf(stdout, "Connection failed with mqtt return code %d\n", (int)returnCode);
                connectionCompletedPromise.set_value(false);
            }
            else
            {
                fprintf(stdout, "Connection completed successfully.\n");
                connectionCompletedPromise.set_value(true);
            }
        }
    };
 
    auto onInterrupted = [&](Mqtt::MqttConnection &, int error) {
        fprintf(stdout, "Connection interrupted with error %s\n", ErrorDebugString(error));
    };
 
    auto onResumed = [&](Mqtt::MqttConnection &, Mqtt::ReturnCode, bool) { fprintf(stdout, "Connection resumed\n"); };
 
    /*
     * Invoked when a disconnect message has completed.
     */
    auto onDisconnect = [&](Mqtt::MqttConnection &) {
        {
            fprintf(stdout, "Disconnect completed\n");
            connectionClosedPromise.set_value();
        }
    };
 
    connection->OnConnectionCompleted = std::move(onConnectionCompleted);
    connection->OnDisconnect = std::move(onDisconnect);
    connection->OnConnectionInterrupted = std::move(onInterrupted);
    connection->OnConnectionResumed = std::move(onResumed);
 
    /*
     * Actually perform the connect dance.
     * This will use default ping behavior of 1 hour and 3 second timeouts.
     * If you want different behavior, those arguments go into slots 3 & 4.
     */
    fprintf(stdout, "Connecting...\n");
    if (!connection->Connect(clientId.c_str(), false, 1000))
    {
        fprintf(stderr, "MQTT Connection failed with error %s\n", ErrorDebugString(connection->LastError()));
        exit(-1);
    }

    if (connectionCompletedPromise.get_future().get())
    {
        while(true){
            // This is invoked upon the receipt of a Publish on a subscribed topic.
            auto onPublish = [&](Mqtt::MqttConnection &, const String &topic, const ByteBuf &byteBuf) {
                const std::string rawJson(reinterpret_cast<char const*> (byteBuf.buffer), byteBuf.len);
                const auto rawJsonLength = static_cast<int>(rawJson.length());
                constexpr bool shouldUseOldWay = false;
                JSONCPP_STRING err;
                Json::Value root;
                   
             //   std::cout << "rawJson is " << rawJson << std::endl;
     
                if (shouldUseOldWay) {
                  Json::Reader reader;
                  reader.parse(rawJson, root);
                } else {
                  Json::CharReaderBuilder builder;
                  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                  if (!reader->parse(rawJson.c_str(), rawJson.c_str() + rawJsonLength, &root,
                                     &err)) {
                    std::cout << "Jsoncpp error in 'awsiot.cpp'" << std::endl;
                    return EXIT_FAILURE;
                  }
                }
                
                iotSampleRateIndex = root["sample_rate_index"].asInt();
                iotLowerBandwidth = root["lower_bandwidth"].asDouble();
                iotUpperBandwidth = root["upper_bandwidth"].asDouble();
                iotDspCutoffFreq = root["cutoff_frequency"].asDouble();
     
                std::cout << "sample rate index: " << iotSampleRateIndex << std::endl;
                std::cout << "amplifier lower bandwidth: " << std::fixed << iotLowerBandwidth << std::endl;
                std::cout << "amplifier upper bandwidth: " << std::fixed << iotUpperBandwidth << std::endl;
                std::cout << "dsp cutoff frequency: " << std::fixed << iotDspCutoffFreq << std::endl;
                
                if(root["message"].asString() == "start"){
                    std::unique_lock<std::mutex> lck(mtx);
                    stop = false;
                    std::cout << "stop is " << stop << std::endl;
                    start_recording.notify_one();
                }else if(root["message"].asString() == "stop"){
                    stop = true;
                    std::cout << "command to stop... stop is " << stop << std::endl;
                }else if(!root["timestamp"].asString().empty()){
                    new_timestamp = root["timestamp"].asString();
                    std::cout << "printing from thread... timestamp is " << new_timestamp << std::endl;
                 //   std::this_thread::sleep_for(std::chrono::seconds(120));
              //  }else if(!current_timestamp.empty() && root["timestamp"].asString().empty()){
               //     stop = true;  
                }else{}

                return EXIT_SUCCESS;
            };
             // Subscribe for incoming publish messages on topic.
      
        std::promise<void> subscribeFinishedPromise;
        auto onSubAck =
            [&](Mqtt::MqttConnection &, uint16_t packetId, const String &topic, Mqtt::QOS QoS, int errorCode) {
                if (errorCode)
                {
                    fprintf(stderr, "Subscribe failed with error %s\n", aws_error_debug_str(errorCode));
                    exit(-1);
                }
                else
                {
                    if (!packetId || QoS == AWS_MQTT_QOS_FAILURE)
                    {
                        fprintf(stderr, "Subscribe rejected by the broker.");
                        exit(-1);
                    }
                    else{}
                }
                subscribeFinishedPromise.set_value();
            };
 
        connection->Subscribe(topic.c_str(), AWS_MQTT_QOS_AT_LEAST_ONCE, onPublish, onSubAck);
        subscribeFinishedPromise.get_future().wait();
    }

 
        /*
         * Unsubscribe from the topic.
         */
        std::promise<void> unsubscribeFinishedPromise;
        connection->Unsubscribe(
            topic.c_str(), [&](Mqtt::MqttConnection &, uint16_t, int) { unsubscribeFinishedPromise.set_value(); });
        unsubscribeFinishedPromise.get_future().wait();
    }
 
    /* Disconnect */
    if (connection->Disconnect())
    {
        connectionClosedPromise.get_future().wait();
    }       
}

void AwsIot::checkTimestamp(){
    while(!stop){
    std::this_thread::sleep_for(std::chrono::seconds(120));
    if(current_timestamp == new_timestamp){
        stop = true;
    }
    else{
        current_timestamp = new_timestamp;
        std::cout << "printing from checkTimestamp... timestamp is " << new_timestamp << std::endl;
        }
}
}


