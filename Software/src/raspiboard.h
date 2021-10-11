//----------------------------------------------------------------------------------
// Rhd2000evalboard.h
//
// Intan Technoloies Rhd2000 Interface API
// Rhd2000EvalBoard Class Header File
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

#ifndef RASPIBOARD_H
#define RASPIBOARD_H

#include <queue>
#include <mutex>
#include <vector>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <sw/redis++/redis++.h>
#include <jsoncpp/json/json.h>
#include "rhd2000registers.h"
//#include "lockfreequeue.h"
#include "vectorbuffer.hpp"
#include "awsiot.h"


class Rhd2000Registers;
using namespace std;
using namespace sw::redis;

//struct ReferenceSource {
    //int stream;
    //int channel;
    //bool softwareMode;
//};

class RaspiBoard
{

public:
    RaspiBoard(int commandLoop);   
    ~RaspiBoard();      
    
    bool stop;
    double realSampleRate;
    vector<char> spiDataBuffer;
    int recordLength;
    atomic<bool> redisReady = false;
    
    // std::string redisHost = "127.0.0.1";
    int redisPort = 6379;    // Optional. The default port is 6379.
    std::string redisHost = "redis.braingeneers.gi.ucsc.edu";
    std::string redisPassword = "z2NNUhDJNW3l3uOiYqfDQ04nwx7Neid08p11Hi18cvA2CPd89BRU8cm0YyCbrDrvn2s6MZICbfoZreWx5RWCDMcNFhLeLLg6N1o";
    std::string deviceName = "data-deepthought";

    atomic<int> spiDataNum = 0;
    //int dataFileSize;
    
    ofstream jsonFile1;
    
   // void initialize(int sampleRateIndex, double desiredLowerBandwidth, double desiredUpperBandwidth, double desiredDspCutoffFreq);

	int open();
	bool uploadFpgaBitfile(string filename);
	


	enum AmplifierSampleRate {          // on RPI3, per channel
	    SampleRate2000Hz,
	    SampleRate3000Hz,
            SampleRate4000Hz,
	    SampleRate5000Hz,
	    SampleRate6250Hz,
	    SampleRate10000Hz,
	    SampleRate12500Hz,
	    SampleRate15000Hz,
	    SampleRate20000Hz,
	    SampleRate25000Hz,
	    SampleRate30000Hz,
	};

	bool setSampleRate(AmplifierSampleRate newSampleRate);
	double getSampleRate() const;
	AmplifierSampleRate getSampleRateEnum() const;
    
	enum AuxCmdSlot {
        AuxCmd1 = 0,
        AuxCmd2 = 1,
        AuxCmd3 = 2,
        AuxCmd4 = 3
	};

	enum BoardPort {
		PortA,
		PortB,
		PortC,
		PortD
	};

	
    void runCommandList(vector<uint16_t> &commandList);
    void printCommandList(const vector<uint16_t> &commandList) const;
    void selectAuxCommandLength(AuxCmdSlot auxCommandSlot, int loopIndex, int endIndex);
    void createAuxCommandList(vector<uint16_t> &commandList, vector<uint16_t> &auxcommandList, AuxCmdSlot auxCommandSlot);
    void runConvertCommandList();   
    void runReadCommandList(vector<char> &raspiReceive, int commandLoop) ;     //for raspiboard_1.cpp
    void saveRegisterValue(vector<char> &raspiReceive);
    void startSaveFile();
    void breakProgram();
    void metadataJson(double boardSampleRate, string expName, double realSampleRate);
    void openMetadataJson(ofstream &outputJFile);
    void writeMetadataJson(ofstream &outputJFile, double realSampleRate);
    void closeMetadataJson(ofstream &outputJFile);
    void saveDataFile();
    void callRedis();

	void resetBoard();
    void resetFpga();
	void setContinuousRunMode(bool continuousMode);
	void setMaxTimeStep(unsigned int maxTimeStep);
	void run();
    bool isRunning();
    unsigned int getNumWordsInFifo();
    unsigned int getLastNumWordsInFifo();
    unsigned int getLastNumWordsInFifo(bool& hasBeenUpdated);
	static unsigned int fifoCapacityInWords();

	void setCableDelay(BoardPort port, int delay);
	void setCableLengthMeters(BoardPort port, double lengthInMeters);
	void setCableLengthFeet(BoardPort port, double lengthInFeet);
	double estimateCableLengthMeters(int delay) const;
	double estimateCableLengthFeet(int delay) const;

	void setDspSettle(bool enabled);
    void setAllDacsToZero();

	enum BoardDataSource {
		PortA1 = 0,
		PortA2 = 1,
		PortB1 = 2,
		PortB2 = 3,
		PortC1 = 4,
		PortC2 = 5,
		PortD1 = 6,
		PortD2 = 7
	};

	void enableDataStream(int stream, bool enabled);
	int getNumEnabledDataStreams() const;

	void getTtlIn(int ttlInArray[]);

	void setDacManual(int value);

	void setLedDisplay(int ledArray[]);
    void setSpiLedDisplay(int ledArray[]);

	void enableDac(int dacChannel, bool enabled);
    void setDacGain(int gain);
    void setAudioNoiseSuppress(int noiseSuppress);
	void selectDacDataStream(int dacChannel, int stream);
	void selectDacDataChannel(int dacChannel, int dataChannel);
    void enableDacHighpassFilter(bool enable);
    void setDacHighpassFilter(double cutoff);
    void setDacThreshold(int dacChannel, int threshold, bool trigPolarity);

	void flush();
//	bool readDataBlock(Rhd2000DataBlock *dataBlock);
    long readDataBlocksRaw(int numBlocks, unsigned char* buffer);
//	bool readDataBlocks(int numBlocks, queue<Rhd2000DataBlock> &dataQueue);
//	int queueToFile(queue<Rhd2000DataBlock> &dataQueue, std::ofstream &saveOut);
    int getBoardMode();
	int getCableDelay(BoardPort port) const;
	void getCableDelay(vector<int> &delays) const;
	void enableDcAmpConvert(bool enable);
	void setExtraStates(unsigned int extraStates);

	void configureChip(Rhd2000Registers *chipRegisters);
	void runDummyCommands(Rhd2000Registers *chipRegisters);
	void configureRegister(Rhd2000Registers *chipRegisters, int reg);

    int readDigitalInManual(bool& expanderBoardDetected);
    void readDigitalInExpManual();

    void setDacRerefSource(int stream, int channel);
    void enableDacReref(bool enabled);

    void resetSequencers();

    // Stimulation sequencer register addresses
    enum StimRegister {
        TriggerParams = 0,
        StimParams = 1,
        EventAmpSettleOn = 2,
        EventAmpSettleOff = 3,
        EventStartStim = 4,
        EventStimPhase2 = 5,
        EventStimPhase3 = 6,
        EventEndStim = 7,
        EventRepeatStim = 8,
        EventChargeRecovOn = 9,
        EventChargeRecovOff = 10,
        EventAmpSettleOnRepeat = 11,
        EventAmpSettleOffRepeat = 12,
        EventEnd = 13,
        DacBaseline = 9,
        DacPositive = 10,
        DacNegative = 11
    };

    enum StimShape {
        Biphasic = 0,
        BiphasicWithInterphaseDelay = 1,
        Triphasic = 2,
        Monophasic = 3
    };

    void setStimCmdMode(bool enabled);
    void programStimReg(int stream, int channel, StimRegister reg, int value);
    void configureStimTrigger(int stream, int channel, int triggerSource, bool triggerEnabled, bool edgeTriggered, bool triggerOnLow);
    void configureStimPulses(int stream, int channel, int numPulses, StimShape shape, bool negStimFirst);
    void setAnalogInTriggerThreshold(double voltageThreshold);
    void setManualStimTrigger(int trigger, bool triggerOn);

    void enableAuxCommandsOnAllStreams();
    void enableAuxCommandsOnOneStream(int stream);

    void setGlobalSettlePolicy(bool settleWholeHeadstageA, bool settleWholeHeadstageB, bool settleWholeHeadstageC,
                               bool settleWholeHeadstageD, bool settleAllHeadstages);

    void setTtlOutMode(bool mode1, bool mode2, bool mode3, bool mode4, bool mode5, bool mode6, bool mode7, bool mode8);
    void setAmpSettleMode(bool useFastSettle);
    void setChargeRecoveryMode(bool useSwitch);
   // awsupload *awsup = new awsupload();

private:
    
    double boardSampleRate;
    int redisSec;  // time of recording chunks for redis
    int fileRecTime;
    void changeSampleRate(int sampleRateIndex, double desiredLowerBandwidth, double desiredUpperBandwidth, double desiredDspCutoffFreq);
    //int numSpiPorts;
    mutex redisMtx;
    vector<char> redisData;
    
    //Rhd2000Registers::ChargeRecoveryCurrentLimit chargeRecoveryCurrentLimit;
    
    Rhd2000Registers *chipRegisters;
    vbuffer * dataQueue = new vbuffer();
    AwsIot *awsiot = new AwsIot();
    boost::lockfree::spsc_queue<int, boost::lockfree::capacity<15000*32*2> > spsc_queue;  
    
   // double chargeRecoveryTargetVoltage;
    //double desiredDspCutoffFreq;
    double actualDspCutoffFreq;
   // double desiredUpperBandwidth;
    double actualUpperBandwidth;
    //double desiredLowerBandwidth;
    double desiredLowerSettleBandwidth;
    double actualLowerBandwidth;
    double actualLowerSettleBandwidth;
    bool dspEnabled;
    
    //double notchFilterFrequency;
    //double notchFilterBandwidth;
    //bool notchFilterEnabled;
    //double highpassFilterFrequency;
    //bool highpassFilterEnabled;
    //double desiredImpedanceFreq;
    //double actualImpedanceFreq;
    //bool impedanceFreqValid;
    //bool useFastSettle;
    //bool headstageGlobalSettle;
    //bool chargeRecoveryMode;
    
    //bool synthMode;
    //bool stimParamsHaveChanged;
  //  ReferenceSource referenceSource;
    
	AmplifierSampleRate sampleRate;
        
};

#endif 

