/*
	 'codesend' hacked from 'send' by @justy
 
	Usage: Voir en dessous
	 (Use RF_Sniffer.ino to check that RF signals are being produced by the RPi's transmitter)

	Compiler :  
	 g++ -c codesend.cpp -o codesend.o
	 g++ codesend.o RCSwitch.o -o codesend -lwiringPi
  */

#include "RCSwitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>

#include "json/json.h"

using namespace std;
using namespace Json;


std::map<std::string, std::string> commandArgs;

Json::Reader 	jsonReader;
Json::Value 	jsonRoot;
Json::Value		jsonSignal;
string btnName;
double startlock_highlow[2];
double endlock_highlow[2];

double zeroEncoding_highlow[2];
double oneEncoding_highlow[2];

char 	remotecode[4096];
string 	remotecodestring;

/** Turns a decimal value to its binary representation */
char* decTobin(uint64_t dec)
{
	static char bin[4096];
	static char binToReturn[4096];
	unsigned int i=0;

	uint64_t entierDecDivisPar2;
	int reste;
	
	while (dec > 0) 
	{
		entierDecDivisPar2 = dec/2;
		reste = dec - 2 * entierDecDivisPar2;
		if (reste == 0) 	bin[i++]='0';
		else 				bin[i++]='1';
		dec=entierDecDivisPar2;
	}

	for (unsigned int j = 0; j< i; j++) 	binToReturn[j] = bin[i-1 - j];
	binToReturn[i] = '\0';

	return binToReturn;
}

/** Turns a decimal value to its binary representation 
 * 
 *  	  binary : 				   b1 b2 b3 b4
 * Padded binary :  0  0  0  0  0  b1 b2 b3 b4
 */
char* dec2binWzerofill(uint64_t dec, unsigned int bitLength)
{
	static char paddedBinToReturn[4096];

	char * decodedBinaryStart = RCSwitch::decTobin(dec);
	int decodedBinLen = strlen(decodedBinaryStart);

	if (decodedBinLen > bitLength) // Impossible de faire la traduction
	{
		for(int i=0;i<bitLength;i++)
			paddedBinToReturn[i]='0';	
		paddedBinToReturn[bitLength] = '\0';
		return paddedBinToReturn;
	}
	
	// copie des bits
	for(int i=0;i<decodedBinLen;i++)
		paddedBinToReturn[bitLength-decodedBinLen+i]=decodedBinaryStart[i];		
	// padding à gauche
	for(int i=0;i<bitLength-decodedBinLen;i++)
		paddedBinToReturn[i]='0';	
	paddedBinToReturn[bitLength]='\0';
	
	return paddedBinToReturn;
}

/* ---------------------------------------------------------------- */
string intToString(int mylong)
{
	string mystring;
	stringstream mystream;
	mystream << mylong;
	return mystream.str();
}

/* ---------------------------------------------------------------- */
int parseCommandLineArgs(int nbArgs, char ** args)
{
	for (int i=1;i<nbArgs; i++)
	{
		if (args[i][0] == '-')
		{
			size_t pos = 0;
			size_t lastpos = 1;
			std::string key="";
			std::string token;
			std::string toparse = args[i];
			std::string delimiter = "=";
			pos = toparse.find(delimiter.c_str(), lastpos) ;
			while ( pos  != std::string::npos) 
			{
				token = toparse.substr(lastpos, pos-1);
				key = key + token;
				lastpos = pos +delimiter.length();
				pos = toparse.find(delimiter.c_str(), lastpos) ;
			}
			std::string value =  toparse.substr(lastpos, toparse.length());
			commandArgs[key]=value;
		}
		else // pas de '-' au début
		{
			std:string key = intToString(i);
			commandArgs[key]=args[i];
		}
	}
	
	return 0;
}

/* ---------------------------------------------------------------- */
void displayCommandLineArgs()
{
	cout << "------------------------------------------------------------------------------------" << endl;
	cout << "------------------------ CommandLine Arguments -------------------" << endl;
	for (std::map<std::string,std::string>::iterator it=commandArgs.begin(); it!=commandArgs.end(); ++it)
	{
    	cout << it->first << " => " << it->second << endl;
	}
	cout << "------------------------------------------------------------------------------------" << endl;
}


/* ---------------------------------------------------------------- */
void diplayRemotes()
{
	cout << "Available remote are the following : " << endl;
	Value::Members members = jsonRoot.getMemberNames();
	for (std::vector<string>::iterator it = members.begin(); it != members.end(); ++it) 
	{
		if (string::npos != (*it).find("remote_"))
			cout << "  " << *it << endl;
	}
}

/* ---------------------------------------------------------------- */
void diplayButtonsForRemote(Value::Members & membersOfSignals)
{
	cout << "Available remote are the following : " << endl;
	for (std::vector<string>::iterator it = membersOfSignals.begin(); it != membersOfSignals.end(); ++it) 
	{
		cout << "  " << *it << endl;
	}
}


/* ---------------------------------------------------------------- */
int main(int argc, char *argv[]) 
{  
    parseCommandLineArgs(argc, argv);
	displayCommandLineArgs();
	
	ifstream jsonFile;
	string rfJsonFilePath = "./radioFrequenceSignalConfig.json";
	jsonFile.open(rfJsonFilePath.c_str());
		
	//bool parsedSuccess = reader.parse(json_example, root, false);
	bool parsedResult = jsonReader.parse(jsonFile, jsonRoot, false);
	jsonFile.close();
	
	if(!parsedResult) // Report failures and their locations in the document.
	{
		cout << "Echec d'analyse du fichier " << rfJsonFilePath << " : " << endl << jsonReader.getFormatedErrorMessages() << endl;
		return 1;
	}
	
	int repeat				= 10;	
	int protocol			= 1;	
	int pin					= 0;
	double pulseLength 		= 0;
	long signalBitNumber	= 0;

   	int decimalcode			= 0;
   	int nbbit				= 24;
   	
   	string remote	 		= "";
   	bool isManchesterCoded	= false;
   	string binarycode		= "";	
    	
	std::map<std::string,std::string>::iterator it;
	it = commandArgs.find("repeat");
	if(it != commandArgs.end())		repeat=atoi(commandArgs["repeat"].c_str());
	
	it = commandArgs.find("remote");
	if(it != commandArgs.end())		remote=commandArgs["remote"];
	else
	{
		cout << "Parameter remote must be set (-remote=<value>) with one of the following values" << endl;
		diplayRemotes();
		return 1;
	}

    // Parse the firt parameter to this command as an integer
    if (wiringPiSetup () == -1) return 1;
	RCSwitch mySwitch = RCSwitch();
	mySwitch.enableTransmit(pin);    
    mySwitch.setRepeatTransmit(repeat);
    mySwitch.setProtocol(protocol);
    
    //-------- Getting protocol signal information from json document ---------//
    Json::Value remoteNode = jsonRoot[remote];    
    if (remoteNode.isNull())
    {
		cout << "Unknown remote" << endl;
		diplayRemotes();
		return 2;
	}
    
    Json::Value protocolNode 		= remoteNode["protocol"];
    if (protocolNode.isNull() || !protocolNode.isInt())
    {
		cout << " Invalid remote properties : for remote:" << remote << " 'protocol' is not set or is not an integer";
		return 3;
	}
    else  {protocol 		= protocolNode.asInt();}
    
    
    Json::Value pulseLengthNode 	= remoteNode["pulseLength"];
    if (pulseLengthNode.isNull()) 	
    {
		cout << " Invalid remote properties : for remote:" << remote << " 'protocol' is not set or is not an integer";
		return 4;
	}
    else 							pulseLength 	= pulseLengthNode.asInt();
    
    Json::Value signalBitNumberNode = remoteNode["SignalBitNumber"];
    if (signalBitNumberNode.isNull()) 	{}
    else 							signalBitNumber = signalBitNumberNode.asInt();

    Json::Value signalsNode = remoteNode["signals"];
    if (signalsNode.isNull()) 		
    {
		cout << " Invalid remote properties : for remote:" << remote << " 'signals' is not or not correctly set";
		return 5;
	}
    else 							
    {
		Value::Members membersOfSignals = signalsNode.getMemberNames();		
		it = commandArgs.find("btn");
		if(it == commandArgs.end())	
		{
			cout << "Parameter 'btn' must be set (-btn=<value>) with one of the following values" << endl;
			diplayButtonsForRemote(membersOfSignals);
			return 6;
		}
		else
		{
			string btnName = commandArgs["btn"];
			jsonSignal = signalsNode[btnName];
			if (jsonSignal.isNull())									
			{
				cout << " Invalid remote properties for remote:" << remote << " button:" << btnName << " Signal code is not set" << endl;
				return 7;
			}
			else if (!jsonSignal.isArray() && (!jsonSignal.isInt())) 	
			{
				cout << " Invalid remote properties for remote:" << remote << " button:" << btnName << " Signal code is neither an array nor an integer" << endl;
				return 8;
			}
		}
	}

	// Assertion sur signalBitNumberNode!=0 ou jsonSignal.isArray()
	if (signalBitNumber == 0 && !jsonSignal.isArray() )
	{
		cout << "Propeties of remote are invalid : either 'signalBitNumberNode' must be set or each button signal must be an array of bit values (0,1) " << endl;
		return 9;
	}

	//------------------------ Getting protocole info for this remote -----------------//
	string protocole = "protocole" + patch::to_string(protocol);
	Json::Value protocoleInfoNode = jsonRoot[protocole];
	if (protocoleInfoNode.isNull())
	{
		cout << " Invalid remote properties for remote: " << remote << " : remote protocol '" << protocole << "' doesn't exist in declared protocols " << endl;
		return 10;
	}

	
	Json::Value  presendSignalCodeTransformationNode = remoteNode["PresendSignalCodeTransformation"];
	if (!presendSignalCodeTransformationNode.isNull())
	{
		string presendSignalCodeTransformationValue = presendSignalCodeTransformationNode.asString();
		if (presendSignalCodeTransformationValue.find("MANCHESTER") != string::npos)
		{isManchesterCoded = true;}
	}
	else
	{
		Json::Value  protocol_presendSignalCodeTransformationNode 	= protocoleInfoNode["PresendSignalCodeTransformation"];
		if (!protocol_presendSignalCodeTransformationNode.isNull() && protocol_presendSignalCodeTransformationNode.asString().find("MANCHESTER") != string::npos)
		{isManchesterCoded = true;}
	}

	//------------------ Getting Start lock HIGH-->LOW pulses ---------//
	Json::Value  startlock_highlowNode 	= remoteNode["startlock_highlow"];
	if (!startlock_highlowNode.isNull()) 	// startlock_highlow est précisé dans les info du button de la remote
	{
		if (!startlock_highlowNode.isArray() || startlock_highlowNode.size() != 2)
		{
			cout << " Invalid remote properties for remote '" << remote << "' : startlock_highlow is not an array or is not well set";
			return 10;
		}
		else
		{
			startlock_highlow[0] = startlock_highlowNode[0].asInt();
			startlock_highlow[1] = startlock_highlowNode[1].asInt();
		}
	}
	else  									// Retrouver le noeud protocole et en déduire startlock_highlow
	{		
		Json::Value startlock_highlowNode =  protocoleInfoNode["startlock_low_NbPulseLength"];
		if (startlock_highlowNode.isNull()) 	
		{
			cout << " Invalid remote properties for remote '" << remote << "' : startlock_highlow array is neither set in btn, nor in protocol"  << endl;
			return 11;
		}
		else if (startlock_highlowNode.isDouble())
		{
			startlock_highlow[0] = pulseLength;
			startlock_highlow[1] = startlock_highlowNode.asDouble() * pulseLength;
		}
		else // must be an integer
		{
			cout << " Invalid remote properties for remote '" << remote << "' : startlock_highlow must be an integer"  << endl;
			return 12;
		}
	}
	
	//------------------ Getting End lock HIGH-->LOW pulses ---------//
	Json::Value  endlock_highlowNode 	= remoteNode["endlock_highlow"];
	if (!endlock_highlowNode.isNull())
	{
		if (!endlock_highlowNode.isArray() || endlock_highlowNode.size() != 2)
		{
			cout << " Invalid remote properties : for remote '" << remote << "' : startlock_highlow is not an array or is not well set";
			return 13;
		}
		else
		{
			endlock_highlow[0] = endlock_highlowNode[0].asInt();
			endlock_highlow[1] = endlock_highlowNode[1].asInt();
		}
	}
	else  									// Retrouver le noeud protocole et en déduire endlock_highlow
	{		
		Json::Value endlock_highlowNode =  protocoleInfoNode["endlock_low_NbPulseLength"];
		if (endlock_highlowNode.isNull()) 	
		{
			cout << " Invalid remote properties for remote '" << remote << "' : endlock_highlow array is neither set in btn, nor in protocol"  << endl;
			return 14;
		}
		else if (endlock_highlowNode.isDouble())
		{
			endlock_highlow[0] = pulseLength;
			endlock_highlow[1] = endlock_highlowNode.asDouble() * pulseLength;
		}
		else // must be an integer
		{
			cout << " Invalid remote properties for remote '" << remote << "' : endlock_highlow must be an integer"  << endl;
			return 15;
		}
	}	
	
	//------------------ Getting zeroEncodingInNbPulse HIGH-->LOW pulses ---------//
	Json::Value  protocol_zeroEncodingInNbPulseNode = protocoleInfoNode["ZeroEncoding_highlow_NbPulseLength"];
	Json::Value  zeroEncodingInNbPulseNode 			= remoteNode["ZeroEncodingInNbPulse"];
	if (!zeroEncodingInNbPulseNode.isNull() && zeroEncodingInNbPulseNode.isArray())
	{
		zeroEncoding_highlow[0] = zeroEncodingInNbPulseNode[0].asDouble();
		zeroEncoding_highlow[1] = zeroEncodingInNbPulseNode[1].asDouble();
	}
	else if (!protocol_zeroEncodingInNbPulseNode.isNull() && protocol_zeroEncodingInNbPulseNode.isArray())
	{
		zeroEncoding_highlow[0] = protocol_zeroEncodingInNbPulseNode[0].asDouble() * pulseLength;
		zeroEncoding_highlow[1] = protocol_zeroEncodingInNbPulseNode[1].asDouble() * pulseLength;
	}
	else
	{
		cout << " Invalid remote properties for remote: " << remote << " : 'zeroEncoding_highlow' parameter must be an array and must be set either in 'remote node' or in 'protocol' node"  << endl;
		return 16;
	}
	
	//------------------ Getting oneEncodingInNbPulse HIGH-->LOW pulses ---------//
	Json::Value  protocol_oneEncodingInNbPulseNode 	= protocoleInfoNode["OneEncoding_highlow_NbPulseLength"];
	Json::Value  oneEncodingInNbPulseNode 			= remoteNode["OneEncodingInNbPulse"];
	if (!oneEncodingInNbPulseNode.isNull() && oneEncodingInNbPulseNode.isArray())
	{
		oneEncoding_highlow[0] = oneEncodingInNbPulseNode[0].asDouble();
		oneEncoding_highlow[1] = oneEncodingInNbPulseNode[1].asDouble();
	}
	else if (!protocol_oneEncodingInNbPulseNode.isNull() && protocol_oneEncodingInNbPulseNode.isArray())
	{
		oneEncoding_highlow[0] = protocol_oneEncodingInNbPulseNode[0].asDouble() * pulseLength;
		oneEncoding_highlow[1] = protocol_oneEncodingInNbPulseNode[1].asDouble() * pulseLength;
	}
	else
	{
		cout << " Invalid remote properties for remote: " << remote << " : 'oneEncoding_highlow' parameter must be an array and must be set in 'remote node' or in 'protocol' node"  << endl;
		return 17;
	}

	//---------------------------------------------- Traitement d'envoi du signal --------------------------------------------------//
	if (jsonSignal.isArray())
	{
		remotecodestring = "";
		for(unsigned int index=0; index<jsonSignal.size(); ++index)  
			remotecodestring.append(jsonSignal[index].asString());	
		strcpy(remotecode,remotecodestring.c_str());
	}
	else // c'est un entier
	{
		decimalcode = jsonSignal.asInt();
		strcpy(remotecode,dec2binWzerofill(decimalcode,signalBitNumber));
	}

	if (isManchesterCoded) // Codage manchester si nécessaire
	{
		string binarycode = "";
    	for(int i=0;i<strlen(remotecode);i++)
    	{
    		if (remotecode[i]=='0')	    		binarycode.append("01");
	    	else if (remotecode[i]=='1')		binarycode.append("10");
	    	else 
	    	{
	    		cout << " Invalid manchester coded signal : " << remotecode << endl;
	    		return 18;
	    	}
    	}
    	strcpy(remotecode,binarycode.c_str());
	}

	// positionner les paramètres de mySwitch : pulseLength, protocole, etc ...
	mySwitch.setRepeatTransmit(repeat);
	mySwitch.setPulseLength(pulseLength);
	mySwitch.setProtocol(protocol);
	mySwitch.setSendZeroEncodingInNbPulse(zeroEncoding_highlow[0]/pulseLength, zeroEncoding_highlow[1]/pulseLength);
	mySwitch.setSendOneEncodingInNbPulse(oneEncoding_highlow[0]/pulseLength, oneEncoding_highlow[1]/pulseLength);

	mySwitch.setSendStartLockHighLowInNbPulse(startlock_highlow[0]/pulseLength, startlock_highlow[1]/pulseLength);
	mySwitch.setSendEndLockHighLowInNbPulse(endlock_highlow[0]/pulseLength, endlock_highlow[1]/pulseLength);


	cout << "> Sending raw code = '" << remotecode << "' > " << endl;
	cout << "  Number of bit   	= " 	<< strlen(remotecode) << endl;
	cout << "  Manchester   		= " 	<< isManchesterCoded << endl;
	cout << "  Protocole 		= " 		<< protocol	<< endl;
	cout << "  ZeroEncoding 		= " 	<< zeroEncoding_highlow[0] 	<< "," << zeroEncoding_highlow[1] 	<< endl;
	cout << "  OneEncoding  		= " 	<< oneEncoding_highlow[0] 	<< "," << oneEncoding_highlow[1] 	<< endl;
	cout << "  StartLock    		= " 	<< startlock_highlow[0] 	<< "," << startlock_highlow[1] 		<< endl;
	cout << "  EndLock   	   	= " 	<< endlock_highlow[0] 		<< "," << endlock_highlow[1] 		<< endl;
	cout << "  Repeat	     	= " 	<< repeat 		<< endl;
	
	cout << mySwitch.computeAndDisplaySignalToSend(remotecode);
	
	
	mySwitch.send(remotecode);
 	cout << "< Sent <" << endl;
	   
	return 0;
}
