//----------------------------------------------------------------------------------
// Rhd2000evalboard.cpp
//
// Intan Technoloies Rhd2000 Interface API
// Rhd2000EvalBoard Class
// Version 1.01 (28 March 2017)
//
// Copyright (c) 2013-2017 Intan Technologies LLC
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the
// use of this software.
//
// Permission is granted to anyone to use this software for any applications that
// use Intan Technologies integrated circuits, and to alter it and redistribute it
// freely.
//
// See http://www.intantech.com for documentation and product information.
//----------------------------------------------------------------------------------

#include <iostream>
#include <unistd.h>
#include <iomanip>
#include <fstream>
#include <vector>
#include <queue>
#include <cmath>
#include <mutex>
#include <ctime>
#include <algorithm>
#include <bitset>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <string>
#include <sw/redis++/redis++.h>
#include <jsoncpp/json/json.h>

#include "raspiboard.h"
extern "C" {
#include "bcm2835.h"
}

#include "rhd2000registers.h"


// Constructor
RaspiBoard::RaspiBoard(int commandLoop)
{
   // redisSec = 6;        // 6 seconds
   // fileRecTime = 2*60;  // 2 minutes, in seconds
   // dataQueue->maxTime = fileRecTime/redisSec;  // this number should always be an integer
   // cout << "Recording time for each dataset is: "<< fileRecTime/60 << " mins" << endl;
    // init board
    bcm2835_init();
    if (!bcm2835_init()) {
      cout << "bcm2835_init failed. Are you running as root??\n" << endl;     // for debug, delete later
    }
    bcm2835_spi_begin();
    if (!bcm2835_spi_begin()) {
      cout << "bcm2835_spi_begin failed. Are you running as root??\n" << endl;
    }
    else {
      cout << "Spi initiated" << endl;
        }
        
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE1);  // Different from Intan chip setting
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
    
    dspEnabled = true;
    
    thread tiot(&AwsIot::iotReadJson, awsiot);
    

    while(true){
        cout << "waiting ..." << endl;
        std::unique_lock<std::mutex> lck(awsiot->mtx);
        awsiot->start_recording.wait(lck);
        
        changeSampleRate(awsiot->iotSampleRateIndex, awsiot->iotLowerBandwidth, awsiot->iotUpperBandwidth, awsiot->iotDspCutoffFreq);
        cout << "sample rate is " << int(boardSampleRate) << "s/s" << endl;
        
       // metadataJson(boardSampleRate);  //add more data into this file
       // thread tchunck(&vbuffer::consumeAll, dataQueue, int(boardSampleRate)*redisSec*32*2);  // chunck size is seconds * channels * 2 bytes per sample
        thread tchunck(&RaspiBoard::saveDataFile, this);
        thread thRedis(&RaspiBoard::callRedis, this);
        thread tcheck(&AwsIot::checkTimestamp, awsiot);
        tcheck.detach(); // use detach because main thread doesn't need to wait for checkTimestamp to finish. In most of the time it sleeps
        runConvertCommandList();
        cout << "End of recording" << endl;
        tchunck.join();
      //  tcheck.join();
        
    }
    
    tiot.join();

}

RaspiBoard::~RaspiBoard()
{
   // bcm2835_spi_end();
   // bcm2835_close();
}

//void RaspiBoard::initialize(int sampleRateIndex, double desiredLowerBandwidth, double desiredUpperBandwidth, double desiredDspCutoffFreq)
//{

//    dspEnabled = true;

//    changeSampleRate(sampleRateIndex, desiredLowerBandwidth, desiredUpperBandwidth, desiredDspCutoffFreq);
//    cout << "sample rate is " << int(boardSampleRate) << "s/s" << endl;
   
   // thread tbreak(&RaspiBoard::breakProgram, this);
   // thread tchunck(&vbuffer::consumeAll, dataQueue, int(boardSampleRate)*10*32*2);  // chunck size is seconds * channels * 2 bytes per sample
    
//    runConvertCommandList();
    
 //   tbreak.join();
  //  tchunck.join();
//}


// Print a command list to the console in readable form.
void RaspiBoard::printCommandList(const vector<uint16_t> &commandList) const
{
    unsigned int i;
    uint16_t cmd;
    int channel, reg, data;
    vector<uint16_t> pcommandList;
    pcommandList = commandList;
    chipRegisters->orderEndianCommandList(pcommandList);

    cout << endl;
    for (i = 0; i < pcommandList.size(); ++i) {
        cmd = pcommandList[i];
        if (cmd < 0 || cmd > 0xffff) {
            cout << "  command[" << i << "] = INVALID COMMAND: " << cmd << endl;
            bitset<16>x(cmd);
            cout << "binary is " << x << endl;
        } else if ((cmd & 0xc000) == 0x0000) {
            channel = (cmd & 0x3f00) >> 8;
            cout << "  command[" << i << "] = CONVERT(" << channel << ")" << endl;
            bitset<16>x(cmd);
            cout << "binary is " << x << endl;
        } else if ((cmd & 0xc000) == 0xc000) {
            reg = (cmd & 0x3f00) >> 8;
            cout << "  command[" << i << "] = READ(" << reg << ")" << endl;
            bitset<16>x(cmd);
            cout << "binary is " << x << endl;
        } else if ((cmd & 0xc000) == 0x8000) {
            reg = (cmd & 0x3f00) >> 8;
            data = (cmd & 0x00ff);
            cout << "  command[" << i << "] = WRITE(" << reg << ",";
            cout << hex << uppercase << internal << setfill('0') << setw(2) << data << nouppercase << dec;
            cout << ")" << endl;
            bitset<16>x(cmd);
            cout << "binary is " << x << endl;
        } else if (cmd == 0x5500) {
            cout << "  command[" << i << "] = CALIBRATE" << endl;
            bitset<16>x(cmd);
            cout << "binary is " << x << endl;
        } else if (cmd == 0x6a00) {
            cout << "  command[" << i << "] = CLEAR" << endl;
            bitset<16>x(cmd);
            cout << "binary is " << x << endl;
        } else {
            cout << "  command[" << i << "] = INVALID COMMAND: ";
            cout << hex << uppercase << internal << setfill('0') << setw(4) << cmd << nouppercase << dec;
            cout << endl;
            bitset<16>x(cmd);
            cout << "binary is " << x << endl;
        }
    }
    cout << endl;
}

// add dummy commands to the end of each aux command slot to make its length to be 8192.
// copy the aux commands to the corresponding section of the command list, then runCommandList
// aux commands and convert commands never run at the same time
void RaspiBoard::createAuxCommandList(vector<uint16_t> &commandList, vector<uint16_t> &auxcommandList, AuxCmdSlot auxCommandSlot)
{
    int auxcommandsize, auxdummysize;      // commandsize must be an constant number so that aux commands can be copied to the right place
    vector<uint16_t> AuxDummyCommand;
    
    auxcommandsize = auxcommandList.size();
    auxdummysize = 64 - auxcommandsize;
    
    if(auxcommandsize < 1 || auxcommandsize > 64) {
            cerr << "Error in RaspiBoard::createAuxCommandList: aux command size out of range." << endl;
            return;
        }
    
    switch(auxCommandSlot) {
        case AuxCmd1:
            chipRegisters->createCommandListDummy(AuxDummyCommand, auxdummysize, (chipRegisters->createRhd2000Command(Rhd2000Registers::Rhd2000CommandRegRead, 42)));
            auxcommandList.insert(auxcommandList.end(), AuxDummyCommand.begin(), AuxDummyCommand.end());
            
            auxcommandsize = auxcommandList.size();       //for debug
            if(auxcommandsize != 64){
                cerr << "Error in RaspiBoard::createAuxCommandList: aux command size is not 60." << endl;
                return;
                }
                
            copy(auxcommandList.begin(), auxcommandList.end(), commandList.begin());
            break;
        case AuxCmd2:
            chipRegisters->createCommandListDummy(AuxDummyCommand, auxdummysize, (chipRegisters->createRhd2000Command(Rhd2000Registers::Rhd2000CommandRegRead, 42)));
            auxcommandList.insert(auxcommandList.end(), AuxDummyCommand.begin(), AuxDummyCommand.end());
            
            auxcommandsize = auxcommandList.size();       //for debug
            if(auxcommandsize != 64){
                cerr << "Error in RaspiBoard::createAuxCommandList: aux command size is not 60." << endl;
                return;
                }
            
            copy(auxcommandList.begin(), auxcommandList.end(), (commandList.begin() + 64));
            break;
        case AuxCmd3:
            chipRegisters->createCommandListDummy(AuxDummyCommand, auxdummysize, (chipRegisters->createRhd2000Command(Rhd2000Registers::Rhd2000CommandRegRead, 42)));
            auxcommandList.insert(auxcommandList.end(), AuxDummyCommand.begin(), AuxDummyCommand.end());
            
            auxcommandsize = auxcommandList.size();       //for debug
            if(auxcommandsize != 64){
                cerr << "Error in RaspiBoard::auxCommandList: aux command size is not 60." << endl;
                return;
                }
            copy(auxcommandList.begin(), auxcommandList.end(), (commandList.begin() + 2*64));
            break;
        case AuxCmd4:
            chipRegisters->createCommandListDummy(AuxDummyCommand, auxdummysize, (chipRegisters->createRhd2000Command(Rhd2000Registers::Rhd2000CommandRegRead, 42)));
            auxcommandList.insert(auxcommandList.end(), AuxDummyCommand.begin(), AuxDummyCommand.end());
            
            auxcommandsize = auxcommandList.size();       //for debug
            if(auxcommandsize != 64){
                cerr << "Error in RaspiBoard::auxCommandList: aux command size is not 8192." << endl;
                return;
                }
            
            copy(auxcommandList.begin(), auxcommandList.end(), (commandList.begin() + 3*64));
            break;
    }
    
    
}

// Run 128 dummy commands (i.e., reading from ROM registers)
void RaspiBoard::runDummyCommands(Rhd2000Registers *chipRegisters)   // test code  
{
    // Create command lists to be uploaded.
    int commandSequenceLength;
    vector<uint16_t> commandList;

    // Create a command list.
    commandSequenceLength = chipRegisters->createCommandListDummy(commandList, 60);
    
    cout << commandSequenceLength << endl;
    
    runCommandList(commandList);

}

// Run command to read the ROM for program TEST
void RaspiBoard::runReadCommandList(vector<char> &raspiReceive, int commandLoop)  //convertCommandLength not used
{
   // Create command lists to be uploaded.
    int commandSequenceLength;
    vector<uint16_t> commandList;

    // read chip id
    commandSequenceLength = chipRegisters->createCommandListDummy(commandList, commandLoop);
 //   printCommandList(commandList);
    
    uint16_t *commInt = &(commandList[0]);
    int commandLen;
    
    commandLen = sizeof(uint16_t)/sizeof(char) * commandSequenceLength;
    
 //   cout << "Read commandLen =" << commandLen << endl;
    
    char raspiRec[2];
    char *commChar = reinterpret_cast<char *>(commInt);
    //get current time
    time_t t = time(0);
    struct tm *now = localtime(&t);
    char bufferI[80];
    strftime(bufferI,80,"I-Read-%Y-%m-%d-%H-%M-%S", now); // yyyy-mm-dd-hh-mm-ss
    ofstream outfileI;
    outfileI.open(bufferI);
    
    cout << "Start reading..." << endl;
    // receive spi data and write to file as unsigned integer          
    for(int i = 0; i < commandLen/2; i ++)
    {
        commChar[0] = commChar[2*i];
        commChar[1] = commChar[2*i+1];
        bcm2835_spi_transfernb(commChar, raspiRec, sizeof(raspiRec)); // void bcm2835_spi_transfernb(char* tbuf, char* rbuf, uint32_t len)
        //outfileI << (short int)(raspiRec[0] << 8 | raspiRec[1]);
        outfileI << raspiRec[0] << raspiRec[1];
    }
    
    
    cout << "Integer file for register reading completed!" << endl;
    outfileI.close();

}

// run commands to sample each channel and save data to a file
void RaspiBoard::runConvertCommandList()
{
   // Create command lists to be uploaded.
    int commandSequenceLength;
    vector<uint16_t> commandList;

    // Create a command list with 32 convert commands
    commandSequenceLength = chipRegisters->createCommandListConvert(commandList);
   // printCommandList(commandList);
     
    uint16_t *commInt = &(commandList[0]);

    int commandLen;
    
    commandLen = sizeof(uint16_t)/sizeof(char) * commandSequenceLength;
    char raspiRec[2];
    char *commChar = reinterpret_cast<char *>(commInt);

   // cout << "command length = " << commandLen << endl;
    cout << "Start recording..." << endl;
    
     auto start = chrono::high_resolution_clock::now();

    while(!awsiot->stop){
        // receive spi data and write to file as raw binary
        for(int i = 0; i < commandLen/2; i ++)
        {
            commChar[0] = commChar[2*i];
            commChar[1] = commChar[2*i+1];
            bcm2835_spi_transfernb(commChar, raspiRec, sizeof(raspiRec));  
            spsc_queue.push(raspiRec[0]);
            spsc_queue.push(raspiRec[1]);
            spiDataNum ++;
            
            //dataQueue->pushData(raspiRec[0]);
            //dataQueue->pushData(raspiRec[1]);
            //dataQueue->count ++;
            
        }
    }
    
    //auto end = chrono::high_resolution_clock::now();
    //auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
   
    //cout << "SPI speed :" << dataQueue->count ++/32/(duration.count()/1000000) << " sample/second per channel" <<endl;
    //realSampleRate = dataQueue->count ++/32/(duration.count()/1000000);
    
    
    //if(awsiot->stop){
        //dataQueue->stopQ = true;
        //}
       
    //cout << "File write compeleted!" << endl;
  //  cout << "Concatenating files to 'start-time_end-time.txt'" << endl;
  //  system("./concat.sh");
  //  system("src/./upload.sh");
  
}

//void RaspiBoard::breakProgram()
//{

	//char ch;
	//ch = getchar();
	//if (ch == 'q' ||ch == 'Q'){
		//stop = true;
        //dataQueue->stopQ = true;
    ////    awsup->stopA = true;
        
	//}
	//else {
		//stop = false;
        //dataQueue->stopQ = false;
     ////   awsup->stopA = false;
	//}
//}

// Run read command to the specific registers and save the value
void RaspiBoard::saveRegisterValue(vector<char> &raspiReceive)  //convertCommandLength not used
{
   // Create command lists to be uploaded.
    int commandSequenceLength;
    vector<uint16_t> commandList;

    // read register value once
    commandSequenceLength = chipRegisters->createCommandListDummy(commandList);
 //   printCommandList(commandList);
    
    uint16_t *commInt = &(commandList[0]);
    int commandLen;
    
    commandLen = sizeof(uint16_t)/sizeof(char) * commandSequenceLength;
    
    char raspiRec[2];
    char *commChar = reinterpret_cast<char *>(commInt);
    //get current time
    time_t t = time(0);
    struct tm *now = localtime(&t);
    char bufferI[80];
    strftime(bufferI,80,"I-Read-%Y-%m-%d-%H-%M-%S", now); // yyyy-mm-dd-hh-mm-ss
    ofstream outfileI;
    outfileI.open(bufferI);
    
    cout << "Start reading..." << endl;
    // receive spi data and write to file as unsigned integer          // Need a function to convert the register value to sample rate
    for(int i = 0; i < commandLen/2; i ++)
    {
        commChar[0] = commChar[2*i];
        commChar[1] = commChar[2*i+1];
        bcm2835_spi_transfernb(commChar, raspiRec, sizeof(raspiRec)); // void bcm2835_spi_transfernb(char* tbuf, char* rbuf, uint32_t len)
        raspiReceive.push_back(raspiRec[0]);
        raspiReceive.push_back(raspiRec[1]);
    }
    
    // translate the received value into readable format
    short int recInt[raspiReceive.size()/2];     // or unsigned int?
    for(int a = 0; a < raspiReceive.size()/2; a++)
    {
        char high = raspiReceive[2*a];
        char low = raspiReceive[2*a+1];
        recInt[a] = (short int)(high << 8 | (low & 0x3F));
     //   cout << recInt[a] << endl;
    }
    
    int sampleRateChn;
    sampleRateChn =chipRegisters->getSampleRate(recInt[2], recInt[3]);    //Spi delay of 2 commands, verify if recInt[3] = adcBufferBias
    outfileI << "Per Channel Sampling Rate is " << sampleRateChn << " S/s" << endl;
    
    cout << "Integer file for register reading write compeleted!" << endl;
    outfileI.close();

}

// General run spi for auxiliary commands
void RaspiBoard::runCommandList(vector<uint16_t> &commandList) // register configuration and temperature sensor etc
{                                                                       
    uint16_t *commInt = &(commandList[0]);
    int commandLen;
    int commandSequenceLength;
    
    commandSequenceLength = commandList.size();
    commandLen = sizeof(uint16_t)/sizeof(char) *  commandSequenceLength;
     //(unsigned) char = 1 byte; unsigned int = 4 byte
    
    char raspiRec[2];
    char *commChar = reinterpret_cast<char *>(commInt);
    
     //// get current time as the file name
    //time_t t = time(0);
    //struct tm *now = localtime(&t);
    //char bufferI[80];
    //strftime(bufferI,80,"I-CMD-%Y-%m-%d-%H-%M-%S", now); // yyyy-mm-dd-hh-mm-ss
    //ofstream outfileI;
    //outfileI.open(bufferI);
    
     for(int i = 0; i < commandLen/2; i ++)
    {
        commChar[0] = commChar[2*i];
        commChar[1] = commChar[2*i+1];
        bcm2835_spi_transfernb(commChar, raspiRec, sizeof(raspiRec));    // transfernb
        //outfileI << (short int)(raspiRec[0] << 8 | raspiRec[1]) << endl;
    }
    
    cout << "intan chip configured" << endl;
    //cout << "Integer file of CMD write compeleted!" << endl;
    //outfileI.close();
}

bool RaspiBoard::setSampleRate(AmplifierSampleRate newSampleRate)    
// Set the raspi clock speed 
// for single channle: 16 * sample rate per channel
// note: there is a latency in the clock because the default spi command length is 8 bits while intan uses
// 16 bits command word. So the latency happens in the middle of each intan command. This makes the actual
// sample rate slower. Moreover, the accuracy of spi clock is unsure.
{
    switch (newSampleRate){
    case SampleRate2000Hz:
     //  bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_65536);
        bcm2835_spi_set_speed_hz(32000*32);
        break;
    case SampleRate3000Hz:
        bcm2835_spi_set_speed_hz(48000*32);
        break;
    case SampleRate4000Hz:
        bcm2835_spi_set_speed_hz(64000*32);
        break;
    case SampleRate5000Hz:
        bcm2835_spi_set_speed_hz(80000*32);
        break;
    case SampleRate6250Hz:
        bcm2835_spi_set_speed_hz(100000*32);
        break;
    case SampleRate10000Hz:
       // bcm2835_spi_set_speed_hz(160000*32);
        bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);
        break;
    case SampleRate12500Hz:
        bcm2835_spi_set_speed_hz(200000*64);      // 64 is not a typo, it was matched to the sample rate
        break;
    case SampleRate15000Hz:
        // bcm2835_spi_set_speed_hz(240000*32);
        bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);
        break;
    case SampleRate20000Hz:                      // the following cases are not used 
        bcm2835_spi_set_speed_hz(320000*32);
        break;
    case SampleRate25000Hz:
        bcm2835_spi_set_speed_hz(400000*32);
        break;
    case SampleRate30000Hz:
        bcm2835_spi_set_speed_hz(480000*32);
        break;
    default:
        return(false);
    }
    return(true);
}


void RaspiBoard::changeSampleRate(int sampleRateIndex, double desiredLowerBandwidth, double desiredUpperBandwidth, double desiredDspCutoffFreq)    // Raspi spi clock speed； sync raspi with amp sample rate
{
    
    switch (sampleRateIndex) {              // for single channel. Write another block to choose single or multi-channel
        case 0:
            boardSampleRate = 2000.0;       
            sampleRate = SampleRate2000Hz;
            break;
        case 1:
            boardSampleRate = 3000.0;        
            sampleRate = SampleRate3000Hz;
            break;
        case 2:
            boardSampleRate = 4000.0;          
            sampleRate = SampleRate4000Hz;
            break;
        case 3:
            boardSampleRate = 5000.0;          
            sampleRate = SampleRate5000Hz;
            break;
        case 4:
            boardSampleRate = 6250.0;          
            sampleRate = SampleRate6250Hz;
            break;
        case 5:
            boardSampleRate = 10000.0;          
            sampleRate = SampleRate10000Hz;
            break;
        case 6:
            boardSampleRate = 12500.0;          
            sampleRate = SampleRate12500Hz;
            break;
        case 7:
            boardSampleRate = 15000.0;          
            sampleRate = SampleRate15000Hz;
            break;
        case 8:
            boardSampleRate = 20000.0;          
            sampleRate = SampleRate20000Hz;
            break;
        case 9:
            boardSampleRate = 25000.0;          
            sampleRate = SampleRate25000Hz;
            break;   
        case 10:
            boardSampleRate = 30000.0;          
            sampleRate = SampleRate30000Hz;
            break;
        }
    
    // Set up an Rhd2000 register object using this sample rate to
    // optimize MUX-related register settings.
    Rhd2000Registers chipRegisters(boardSampleRate);  // set the on chip adc sample rate to be the match the board
    
    int commandSequenceLength;
    vector<uint16_t> commandList;
    vector<uint16_t> auxcommandList;
     
    setSampleRate(sampleRate);
        
    // Before generating register configuration command sequences, set amplifier
    // bandwidth paramters.
    actualDspCutoffFreq = chipRegisters.setDspCutoffFreq(desiredDspCutoffFreq);
    actualLowerBandwidth = chipRegisters.setLowerBandwidth(desiredLowerBandwidth);
    actualUpperBandwidth = chipRegisters.setUpperBandwidth(desiredUpperBandwidth);
    chipRegisters.enableDsp(dspEnabled);
    
    if (dspEnabled) {
        cout.precision(2);
        cout << "Desired/Actual DSP Cutoff: " << fixed << desiredDspCutoffFreq << " Hz / " << actualDspCutoffFreq << " Hz" << endl;
    } else {
        cout << "Desired/Actual DSP Cutoff: DSP disabled" << endl;
    }

    cout << "Desired/Actual Lower Bandwidth: " << fixed << desiredLowerBandwidth << " Hz / " << actualLowerBandwidth << " Hz" << endl;
    cout << "Desired/Actual Lower Amp Settle Bandwidth: " << desiredLowerSettleBandwidth << " Hz / " << actualLowerSettleBandwidth << " Hz" << endl;
    cout << "Desired/Actual Upper Bandwidth: " << fixed << desiredUpperBandwidth / 1000.0 << " kHz / " << actualUpperBandwidth / 1000.0 << " kHz" << endl;
    
     
         chipRegisters.createCommandListDummy(commandList, 64*4, chipRegisters.createRhd2000Command(Rhd2000Registers::Rhd2000CommandRegRead, 44)); // create a dummy command list for commandList vector, later copy the right commands to the corresponding slot
        
         // Create a list of 60 commands to program most RAM registers on a RHD2000 chip
         commandSequenceLength = chipRegisters.createCommandListRegisterConfig(auxcommandList, false);
         createAuxCommandList(commandList, auxcommandList, AuxCmd1); //config the first auxcmd
          // Create a list of 0 commands to sample auxiliary ADC inputs, temperature sensor, and supply voltage sensor
         commandSequenceLength = chipRegisters.createCommandListTempSensor(auxcommandList);
         createAuxCommandList(commandList, auxcommandList, AuxCmd2); //config the second auxcmd  (maybe not)
                
         // Next, fill the other two command slots with dummy commands.  -- change to fill all four with dummy commands.
         chipRegisters.createCommandListDummy(auxcommandList, 60, chipRegisters.createRhd2000Command(Rhd2000Registers::Rhd2000CommandRegRead, 41));
         createAuxCommandList(commandList, auxcommandList, AuxCmd3);
         chipRegisters.createCommandListDummy(auxcommandList, 60, chipRegisters.createRhd2000Command(Rhd2000Registers::Rhd2000CommandRegRead, 40));
         createAuxCommandList(commandList, auxcommandList, AuxCmd4);
      //  printCommandList(commandList);
        // run aux commands
         runCommandList(commandList);
     
}

// save metadata to a json file before recording. Can add more values 
void RaspiBoard::metadataJson(double boardSampleRate, string expName, double realSampleRate)
{
    Json::Value root;
    Json::StreamWriterBuilder builder;
    const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    root["version"] = "0.0.1";
    root["name"] = expName;
   
    time_t t = time(0);
    struct tm *now = localtime(&t);
    char cntime[80];
    strftime(cntime, 80, "%Y-%m-%d-%H-%M-%S", now);
    root["timestamp"] = cntime;  // cntime is a char pointer, this may cause problem
    
    root["sample_rate"] = boardSampleRate;
    root["real_sample_rate"] = realSampleRate;
    root["recording_length"] = recordLength;
    root["hardware"] = "Raspi RHD Controller Rev2";
    root["num_channels"] = 32;
    
    string outputFName(cntime);
    
    ofstream outputJFile("raspi-" + outputFName + ".config");
  //  outputJFile.open("%s", beginName, cntime, "%s", fileType);
    writer->write(root, &outputJFile);
    outputJFile.close();

}

// open an Json to record real sample rate for each data file 
void RaspiBoard::openMetadataJson(ofstream &outputJFile)
{
    time_t t = time(0);
    struct tm *now = localtime(&t);
    char cntime[80];
    strftime(cntime, 80, "%Y-%m-%d-%H-%M-%S", now);
    
    string outputFName(cntime);
    
    outputJFile.open(outputFName + "-RealSampleRate.Json");

}

void RaspiBoard::writeMetadataJson(ofstream &outputJFile, double realSampleRate)
{
    Json::Value root;
    Json::StreamWriterBuilder builder;
    const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter()); 
    
    time_t t = time(0);
    struct tm *now = localtime(&t);
    char cntime[80];
    strftime(cntime, 80, "%H-%M-%S", now);
    
    root["timestamp"] = cntime;  // cntime is a char pointer, this may cause problem
    root["real_sample_rate"] = realSampleRate;
    
    writer->write(root, &outputJFile);
   // cout << "json written" << endl;
}

void RaspiBoard::closeMetadataJson(ofstream &outputJFile)
{
    outputJFile.close();
}

void RaspiBoard::saveDataFile()
{
    char spiBufferValue;
    time_t t = time(0);
    struct tm *now = localtime(&t);
    char buffer[80];
    strftime(buffer, 80, "raspi-%Y-%m-%d-%H-%M-%S", now);
    ofstream outfile;
    outfile.open(buffer);
    
    int redisBufferSize = int(boardSampleRate)*32*12;
    char redisBuffer[redisBufferSize];
    int tailDataNum = 0;
    int dataFileSize = int(boardSampleRate)*32*20;     // sampleRate*channels*duration
    
    int redisBufferCounter = 0;                          // initiate buffer counter
    int outfileCounter = 0;                     
    openMetadataJson(jsonFile1);
    writeMetadataJson(jsonFile1, 0.0);                 // to initiate the beginning time
    
    auto start = chrono::high_resolution_clock::now();
    
    while(!awsiot->stop){
            
            if(spsc_queue.pop(spiBufferValue)) {
                    outfile << spiBufferValue;
                    redisBuffer[redisBufferCounter] = spiBufferValue;
                    outfileCounter += 1;
                    redisBufferCounter += 1;
                }
                
            if(outfileCounter == dataFileSize*2){
                cout << "start another saving file" << endl;
                spiDataNum = 0;
                outfileCounter = 0;
                outfile.close();
                
                auto end = chrono::high_resolution_clock::now();
                auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                realSampleRate = dataFileSize/32/static_cast<double>(duration.count()/1000000);
               // cout << duration.count() << endl;
               // cout << realSampleRate << endl;
                
                writeMetadataJson(jsonFile1, realSampleRate);
                    
                time_t t = time(0);
                struct tm *now = localtime(&t);
                //char buffer[80];
                strftime(buffer, 80, "raspi-%Y-%m-%d-%H-%M-%S", now);

                outfile.open(buffer);
                
                start = chrono::high_resolution_clock::now();
                }
             if(redisBufferCounter ==  redisBufferSize){
                redisBufferCounter = 0;
                redisMtx.lock();
                redisData.assign(redisBuffer, redisBuffer+redisBufferSize);
                redisMtx.unlock();
             //   cout << "redis Data Size = " << redisData.size() << endl;
                redisReady = true;
                }
            
    }
    
    if(awsiot->stop){
        cout << "saving the tail" << endl;                       
        
     //   auto start = chrono::high_resolution_clock::now();
        
        while(spsc_queue.pop(spiBufferValue)){
        outfile << spiBufferValue;
        tailDataNum ++;
        }
    
        
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
        realSampleRate = spiDataNum/32/static_cast<double>(duration.count()/1000000);
        writeMetadataJson(jsonFile1, realSampleRate);
        cout << "real Sample Rate = " << realSampleRate << endl;
        cout << "tailDataNum = " << static_cast<int>(spiDataNum) << endl;
        cout << "duration = " << duration.count() << endl;
        cout << "tail remaining = " << tailDataNum << endl;
        
        outfile.close();
    }
    
    
    closeMetadataJson(jsonFile1);
    cout << "Binary file written!" << endl;
    outfile.close();
    tailDataNum = 0;
}

void RaspiBoard::callRedis()
{
   
    while(!awsiot->stop){
  
    if (redisReady){
    //    cout << "redis is ready" << endl;
        // auto start = chrono::high_resolution_clock::now(); 
        //string s(redisData, redisData+(sizeof(redisData)/sizeof(char)));   
        string s(redisData.begin(), redisData.begin()+redisData.size());  
        using Attrs = std::vector<pair<string, string>>;
        Attrs attrs = { {"x", s} };
        
    try{
        // set up redis
        ConnectionOptions connection_options;
        connection_options.host = redisHost;
        connection_options.port = redisPort; 
       // connection_options.password = redisPassword;   // Optional. No password by default.
        Redis redis(connection_options);
        //cout << "redis initialized" << endl;
      
        auto id = redis.xadd(deviceName, "*", attrs.begin(), attrs.end());
        cout << "redis stream done" << endl;
        //auto end = chrono::high_resolution_clock::now();
        //auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
        //cout << "time cost - redis: " << duration.count() << " us" << endl;
        
        redisReady = false;
        
        }catch(const Error &e) {
            cout << "Redis stream failed: " ;
            cerr << e.what() << endl;
            }
        }
        else{}
    }
}


