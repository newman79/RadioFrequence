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

using namespace std;

std::map<std::string, std::string> commandArgs;

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
int main(int argc, char *argv[]) 
{  
    parseCommandLineArgs(argc, argv);
	displayCommandLineArgs();
	
	int repeat						= 10;	
	int protocol					= 1;	
	int pin							= 0;	

   	int decimalcode			= 0;
   	int nbbit						= 24;
   	
   	string manchbin 			= "";
   	string binarycode		= "";	
    	
    // This pin is not the first pin on the RPi GPIO header!
    // Consult https://projects.drogon.net/raspberry-pi/wiringpi/pins/
    // for more information.
	
	std::map<std::string,std::string>::iterator it;
	it = commandArgs.find("repeat");
	if(it != commandArgs.end())		repeat=atoi(commandArgs["repeat"].c_str());
	
	it = commandArgs.find("protocol");
	if(it != commandArgs.end())		protocol=atoi(commandArgs["protocol"].c_str());
	
	it = commandArgs.find("pin");
	if(it != commandArgs.end())		pin=atoi(commandArgs["pin"].c_str());
	
	it = commandArgs.find("dec");
	if(it != commandArgs.end())		decimalcode=atoi(commandArgs["dec"].c_str());

	it = commandArgs.find("nbbit");
	if(it != commandArgs.end())		nbbit=atoi(commandArgs["nbbit"].c_str());
	
	it = commandArgs.find("bin");
	if(it != commandArgs.end())		binarycode=commandArgs["bin"];

	it = commandArgs.find("manchbin");
	if(it != commandArgs.end())		manchbin=commandArgs["manchbin"];
	
	// Assertion arguments bien positionnés
	int isSet = 0;
	if (decimalcode !=0) 				isSet++;
	if (binarycode.length() !=0)  	isSet++;
	if (manchbin.length() !=0) 	isSet++;
	if (isSet != 1)
	{
		cout << " One and only one of the following argument must be set : dec (with nbbit), bin,  manchbin" << endl;
		return 1;
	}
	// Assertion si décimal code set, alors nbbit doit l'etre
	if (decimalcode !=0 && nbbit==0)
	{
		cout << " dec is set without nbbit ; nbbit must be set too ..." << endl;
		return 2;
	}
	
	// Assertion protocol
	if (protocol < 1 || protocol > 3)
	{
		cout << " protocol must set between 1 and 3" << endl;
		return 3;
	}
	
    // Parse the firt parameter to this command as an integer
    if (wiringPiSetup () == -1) return 1;
	RCSwitch mySwitch = RCSwitch();
	mySwitch.enableTransmit(pin);    
    mySwitch.setRepeatTransmit(repeat);
    mySwitch.setProtocol(protocol);
    
    if (decimalcode != 0)
    {
		cout << "sending decimal code='" << decimalcode << "'  ...";
   	 	mySwitch.send(decimalcode, nbbit);
   	 	cout << " sent" << endl;
    }
    else if (binarycode.length() != 0)
    {
		cout << "sending binary code " << binarycode << "...";
    	mySwitch.send((char*)binarycode.c_str());
   	 	cout << " sent" << endl;
    }
    else // manchesterCode
    {
    	binarycode = "";
    	const char * manchesterCode = manchbin.c_str();
    	for(int i=0;i<manchbin.length();i++)
    	{
    		if (manchesterCode[i]=='0')	    		binarycode.append("01");
	    	else if (manchesterCode[i]=='1')		binarycode.append("10");
	    	else 
	    	{
	    		cout << " Invalid manchester coded signal : " << manchbin << endl;
	    		return 4;
	    	}
    	}
		cout << "sending manchester binary code=" << manchbin << " true code is=" << binarycode << "...";
    	mySwitch.send((char*)binarycode.c_str());
   	 	cout << " sent" << endl;
    }
	return 0;
}

/*
	-------------------------- Protocol 3 : Interrupteur sans fil Dio By CHACON  -------------------------- 
		VerrouDebutSignal =  HIGH[200us] --> LOW[2810us] --> signal = 64 bits manchester codant un code de 32 bits --> Verrou de fin HIGH[240us] --> LOW[10704us]	
					pulseLength=250us
	    			Bit codé par manchester : pour transmettre 1 envoyer 1 puis 0, pour transmettre 0 envoyer 0 puis 1
	    			Envoi d'un 1 : HIGH[250us] --> LOW[1.26*pulseLength]
	    			Envoi d'un 0 : HIGH[250us] --> LOW[5.5*pulseLength]
	    			Remarque : lors de l'analyse, 1er bit du signal =   HIGH[140us] --> LOW[305us]    (le high de tous les autres bit dure toujours 250 us)
	    								mais un send avec 1er bit = HIGH[250us] --> LOW[305us]    marche bien avec le matériel
	    			
		Signal ON 	: 		manchester(64 bits)   	1001101001011010010110011010011001010101010101100110100110101010     	true code(32 bits)=10110011001011010000000101101111
		Signal OFF 	:		manchester(64 bits) 	1001101001011010010110011010011001010101010101100110101010101010		true code(32 bits)=10110011001011010000000101111111
		Pour envoyer le signal qui émule un appui sur le 1 de l'interrupteur : 																		sudo ./codesend64 -protocol=3 -repeat=10 -pin=0 -bin=1001101001011010010110011010011001010101010101100110100110101010		
			ou
			sudo ./codesend64 -protocol=3 -repeat=10 -pin=0 -manchbin=10110011001011010000000101101111			
	-------------------------- Protocol 3 : Interrupteur sans fil 2 Dio By CHACON  -------------------------- 
		Je n'ai pas encore enregistré les 2 signaux envoyés par l'interrupteur (via RFSniffer ou RadioReception  sudo ./radioReceptionOrig aaa.php 2 )
		
		
	-------------------------- Protocol 1 : Triple interrupteur sans fil 1 marque FUNRY --------------------------	
	    VerrouDebutSignal = HIGH[200us] --> LOW[11165us] --> Signal sur 24 bits --> Verrou de fin ?
	    pulseLength=VerrouDebutSignal.DureeLOW/31
		Codage d'un 1 : HIGH[3*pulseLength] --> LOW[1*pulseLength]
		Codage d'un 0 : HIGH[1*pulseLength] --> LOW[3*pulseLength]
		VerrouFinSignal = HIGH[200us] --> LOW[11165 +- 100 us]
	    
		Pour émuler bouton D de la télécommande qui pilote le tripe interrupteur tactile													sudo ./codesend64 -protocol=1 -repeat=10 -pin=0 -dec=3258513 -nbbit=24
		Pour émuler bouton B de la télécommande qui pilote le tripe interrupteur tactile													sudo ./codesend64 -protocol=1 -repeat=10 -pin=0 -dec=3258516 -nbbit=24
		Pour émuler bouton A de la télécommande qui pilote le tripe interrupteur tactile													sudo ./codesend64 -protocol=1 -repeat=10 -pin=0 -dec=3258520 -nbbit=24
		Pour émuler bouton C de la télécommande qui pilote le tripe interrupteur tactile													sudo ./codesend64 -protocol=1 -repeat=10 -pin=0 -dec=3258514 -nbbit=24
	-------------------------- Protocol 1 : Interrupteur sans fil 2 marque FUNRY  -------------------------- 
		Je n'ai pas encore enregistré la télécommande (via RFSniffer ou RadioReception  sudo ./radioReceptionOrig aaa.php 2 )
		Remarque : l'interrupteur présente un défaut, il ne capte pas les signaux radios
		
		
	-------------------------- 	Protocol 1 : Prise CHACON -------------------------- 
	    VerrouDebutSignal = HIGH[200us] --> LOW[13318us] --> Signal sur 24 bits --> Verrou de fin ?
	    pulseLength=VerrouDebutSignal.DureeLOW/31
		Codage d'un 1 : HIGH[3*pulseLength] --> LOW[1*pulseLength]
		Codage d'un 0 : HIGH[1*pulseLength] --> LOW[3*pulseLength]
		VerrouFinSignal = HIGH[200us] --> LOW[13318 +- 100 us]

		Categorie1 BTN1 ON/OFF			1381717/1381716			00010101 00010101 01010101 / 00010101 00010101 01010100
		Categorie1 BTN2 ON/OFF			1394005/1394004			00010101 01000101 01010101 / 
		Categorie1 BTN3 ON/OFF			1397077/1397076			00010101 01010001 01010101 / 
		Categorie1 BTN4 ON/OFF			1397845/1397844			00010101 01010100 01010101 / 

		Categorie2 BTN1 ON/OFF			4527445/4527444			01000101 00010101 01010101 / 01000101 00010101 01010100
		Categorie2 BTN2 ON/OFF			4539733/4539732			01000101 01000101 01010101 / 01000101 01000101 01010100
		Categorie2 BTN3 ON/OFF			4542805/4542804			01000101 01010001 01010101 / 
		Categorie2 BTN4 ON/OFF			4543573/4543572			01000101 01010100 01010101 / 

		Categorie3 BTN1 ON/OFF			5313877/5313876			01010001 00010101 01010101 / 01010001 00010101 01010100
		Categorie3 BTN2 ON/OFF			5326165/5326164			
		Categorie3 BTN3 ON/OFF			5329237/5329236			
		Categorie3 BTN4 ON/OFF			5330005/5330004			

		Categorie4 BTN1 ON/OFF			5510485/5510484			01010100 00010101 01010101 / 01010100 00010101 01010100
		Categorie4 BTN2 ON/OFF			5522773/5522772			
		Categorie4 BTN3 ON/OFF			5525845/5525844			
		Categorie4 BTN4 ON/OFF			5526613/5526612			
	
		==> Analyse plus poussée : On suspecte que code = CodeCategorie(8octets) + CodeBouton(8octets) + CodeEtat(8octets)
*/
