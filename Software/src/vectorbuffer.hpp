#include <iostream>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <fstream>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <ctime>
#include <string>
#include <sw/redis++/redis++.h>

using namespace std;
using namespace sw::redis;

class vbuffer{
public:
    vector<char> mydata;
    int count = 1;
    bool stopQ = false;
    int recTime = 0;
    int maxTime;
    ofstream outfile;
    
    
    vbuffer():ano(0){}
    
    int pushData(const char &data){
        mydata.push_back(data);
      //  cout << "call push" << count << endl;
        return mydata.size();
    }
    
    void consumeAll(unsigned int num){
        cout << "save local file" << endl;
       //// save local file
        //time_t t = time(0);
        //struct tm *now = localtime(&t);
        //char buffer[80];
        //strftime(buffer, 80, "%Y-%m-%d-%H-%M-%S.txt", now);
        //ofstream outfile;
        //outfile.open(buffer);
        
        while(!stopQ){
          
          if (mydata.size() >= num){
            
            string s(mydata.begin(), mydata.begin()+num);
            using Attrs = vector<pair<string, string>>;
            Attrs attrs = { {"x", s} };
            
            auto start = chrono::high_resolution_clock::now();
            try{
                // set up redis
                ConnectionOptions connection_options;
                connection_options.host = "67.58.49.54";  // Required.
            	//connection_options.host = "127.0.0.1";
                connection_options.port = 6379; // Optional. The default port is 6379.
                connection_options.password = "z2NNUhDJNW3l3uOiYqfDQ04nwx7Neid08p11Hi18cvA2CPd89BRU8cm0YyCbrDrvn2s6MZICbfoZreWx5RWCDMcNFhLeLLg6N1o";   // Optional. No password by default.
                Redis redis(connection_options);
                cout << "redis initialized" << endl;
              
                auto id = redis.xadd("data-marvin", "*", attrs.begin(), attrs.end());
                cout << "redis stream" << endl;
                auto end = chrono::high_resolution_clock::now();
                auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                cout << "time cost - redis: " << duration.count() << " us" << endl;
            }catch(const Error &e) {
                cout << "stream failed: " ;
                cerr << e.what() << endl;
              }
            
          //  auto start2 = chrono::high_resolution_clock::now();
            
            
            // Double check this part to make sure it works as expected
              if (recTime == 0){      
                 // save local file with new file name
                time_t t = time(0);
                struct tm *now = localtime(&t);
                char buffer[80];
                strftime(buffer, 80, "piphys-%Y-%m-%d-%H-%M-%S", now);
                outfile.open(buffer);
                outfile.write((char *)&mydata[0], num);   // See what happens if change &mydata[0] to &mydata[2]
                recTime += 1;
                mydata.erase(mydata.begin(), mydata.begin()+num);
              }else if(recTime == maxTime){
                // save two more data before close the file -- just need to do this for the very first file
                outfile.close();
              }else{
                outfile.write((char *)&mydata[0], num);
                recTime += 1;
                mydata.erase(mydata.begin(), mydata.begin()+num);
              }
                
            
          }else{
            sleep(1);
          }
        }
         
    }

private:
    atomic<int> ano;
};




