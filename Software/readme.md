# Raspberry Pi - intan RHD Software with IoT interface


## Enviornment Setup:
  Libraries and dependencies are installed to default path /usr/local
- ### bcm2835 
  C library for Broadcom BCM 2835 to access SPI. *[Link](https://www.airspayce.com/mikem/bcm2835/)*    
  
  __Install:__
  ```
  # download the latest version of the library, say bcm2835-1.xx.tar.gz, then:
  tar zxvf bcm2835-1.xx.tar.gz 
  cd bcm2835-1.xx
  ./configure
  make
  sudo make check
  sudo make install     
  ```

- ### redis-plus-plus
  C++ client library for Redis. *[Link](https://github.com/sewenew/redis-plus-plus)*  
  
  __Install:__
  ```
  # first install hiredis
  git clone https://github.com/redis/hiredis.git
  cd hiredis
  make
  make install
  
  # then install redis-plus-plus with cmake
  git clone https://github.com/sewenew/redis-plus-plus.git
  cd redis-plus-plus
  mkdir compile
  cd compile
  cmake -DCMAKE_BUILD_TYPE=Release ..
  make
  make install
  cd ..
  ```
  
- ### JsonCpp
  C++ library to manipulate Json files. *[Link](https://github.com/open-source-parsers/jsoncpp)*
  
  __Install:__
  ```
  sudo apt-get install libjsoncpp-dev
  ```
  - __Note:__ Using the vcpkg dependency manager as said in the instruction on JsonCpp's github page may stuck the pi. The header file using *apt-get install* should be included as <jsoncpp/json/json.h>.

- ### AWS IOT Cpp
  Library and examples for using AWS IOT device. *[Link](https://github.com/aws/aws-iot-device-sdk-cpp-v2)*
  
  __Install:__
  ```
  mkdir sdk-cpp-workspace
  cd sdk-cpp-workspace
  git clone --recursive https://github.com/aws/aws-iot-device-sdk-cpp-v2.git
  mkdir aws-iot-device-sdk-cpp-v2-build
  cd aws-iot-device-sdk-cpp-v2-build
  cmake -DCMAKE_INSTALL_PREFIX="/usr/local"  -DBUILD_DEPS=ON ../aws-iot-device-sdk-cpp-v2
  cmake --build . --target install   
  ```

- ### AWS S3/prp
  Long term data file storage.
  
  __Install:__
  ```
  pip install awscli-plugin-endpoint
  ```
  __Credentials:__ 
  Find or ask Key and Secret on *prp-dash-helpdesk* channel on Slack then put them at ~/.aws/credentials
  
  __Check the endpoint:__
  ```
  aws --profile prp --endpoint https://s3.nautilus.optiputer.net s3 ls s3://braingeneers/archive/ephysDebugging/
  ```

 ## Application Compile and Run   

  __Compile:__
  ```
  mkdir build
  cd build
  cmake -DCMAKE_PREFIX_PATH=/usr/local  -DCMAKE_BUILD_TYPE=Release ..
  cmake --build . --config Release
  cd ..
  ```

  __Run:__
  ```
  sudo ./rhd_datastream
  ```
<!-- Json rules for AWS IOT: -->




