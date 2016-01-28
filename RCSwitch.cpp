/*
  RCSwitch - Arduino libary for remote control outlet switches
  Copyright (c) 2011 Suat Özgür.  All right reserved.
  
  Contributors:
  - Andre Koehler / info(at)tomate-online(dot)de
  - Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
  - Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=48
  
  Project home: http://code.google.com/p/rc-switch/

  This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with this library; if not, 
  write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "RCSwitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <cstring>


uint64_t 	RCSwitch::nReceivedValue 		= 0;
unsigned int RCSwitch::nReceivedBitlength 	= 0;
unsigned int RCSwitch::nReceivedDelay 		= 0;
unsigned int RCSwitch::nReceivedProtocol 	= 0;
unsigned int RCSwitch::timings[RCSWITCH_MAX_CHANGES];
int RCSwitch::nReceiveTolerance 			= 60;
unsigned int RCSwitch::changeCount 			= 0;

RCSwitch::RCSwitch() 
{
	this->nReceiverInterrupt 	= -1;
	this->nTransmitterPin 		= -1;
	RCSwitch::nReceivedValue 	= 0;
	this->setPulseLength(350);
	this->setRepeatTransmit(10);
	this->setReceiveTolerance(60);
	this->setProtocol(1);
}

/** Sets the protocol to send. */
void RCSwitch::setProtocol(int nProtocol) 
{
	this->nProtocol = nProtocol;
	if (nProtocol == 1)  		{ this->setPulseLength(350); }
	else if (nProtocol == 2) 	{ this->setPulseLength(650); }
	else if (nProtocol == 3) 	{ this->setPulseLength(250); }
 }
/** Sets the protocol to send with pulse length in microseconds.  */
void RCSwitch::setProtocol(int nProtocol, int nPulseLength) 
{
	this->nProtocol = nProtocol;
	this->setPulseLength(nPulseLength); 
}
/** Sets pulse length in microseconds  */
void RCSwitch::setPulseLength(int nPulseLength) {this->nPulseLength = nPulseLength;}
/** Sets Repeat Transmits */
void RCSwitch::setRepeatTransmit(int nRepeatTransmit) {this->nRepeatTransmit = nRepeatTransmit;}
/** Set Receiving Tolerance */
void RCSwitch::setReceiveTolerance(int nPercent) {RCSwitch::nReceiveTolerance = nPercent;}
  
/**
 * Enable transmissions
 * @param nTransmitterPin    Arduino Pin to which the sender is connected to
 */
void RCSwitch::enableTransmit(int nTransmitterPin) 
{
  this->nTransmitterPin = nTransmitterPin;
  pinMode(this->nTransmitterPin, OUTPUT);
}

/** Disable transmissions  */
void RCSwitch::disableTransmit() { this->nTransmitterPin = -1;}

/** ________________________________________________________________________________________________________________________________________________________  */
void RCSwitch::send(unsigned long Code, unsigned int length) 
{ 
	this->send( this->dec2binWzerofill(Code, length) );
}

/** ________________________________________________________________________________________________________________________________________________________  */
std::string RCSwitch::computeAndDisplaySignalToSend(char * sCodeWord)
{
	std::string result="";
	float * addr0or1EncodingInNbPulse;
	
	result += "VS[" + patch::to_string(startLockHighLowInNbPulse[0]*nPulseLength) + ",";
	result += patch::to_string(startLockHighLowInNbPulse[1]*nPulseLength) + "]\n";
	
	int i=0;
	while (sCodeWord[i] != '\0') 
	{
		switch(sCodeWord[i]) 
		{
			case '0': addr0or1EncodingInNbPulse=zeroEncodingHighLowInNbPulse;
					break;
			case '1': addr0or1EncodingInNbPulse=oneEncodingHighLowInNbPulse;
					break;
		}
		result += "(" + patch::to_string(addr0or1EncodingInNbPulse[0]*nPulseLength) + ",";
		result += patch::to_string(addr0or1EncodingInNbPulse[1]*nPulseLength) + ")";
		i++;
		if (i%8==0)	{result += "\n";}
	}
	
	result += "VE[" + patch::to_string(endLockHighLowInNbPulse[0]*nPulseLength) + ",";
	result += patch::to_string(endLockHighLowInNbPulse[1]*nPulseLength) + "]";
	
	return result;
}

/** ________________________________________________________________________________________________________________________________________________________  */
void RCSwitch::send(char* sCodeWord) 
{
	for (int nRepeat=0; nRepeat<nRepeatTransmit; nRepeat++) 
	{
		int i = 0;
		this->sendStartSignal(); // verrou de début du signal
		while (sCodeWord[i] != '\0') 
		{
		  switch(sCodeWord[i]) 
		  {
			case '0': this->send0();
					break;
			case '1': this->send1();
					break;
		  }
		  i++;
		}
		this->sendEndSignal();
	}
}

/** ________________________________________________________________________________________________________________________________________________________  */
void RCSwitch::transmit(float nHighPulses, float nLowPulses) 
{
    boolean disabled_Receive = false;
    int nReceiverInterrupt_backup = nReceiverInterrupt;
    if (this->nTransmitterPin != -1) 
    {
        if (this->nReceiverInterrupt != -1)  { this->disableReceive();  disabled_Receive = true;}
        
        digitalWrite(this->nTransmitterPin, HIGH);
        delayMicroseconds( this->nPulseLength * nHighPulses);
        digitalWrite(this->nTransmitterPin, LOW);
        delayMicroseconds( this->nPulseLength * nLowPulses);
        
        if (disabled_Receive){ this->enableReceive(nReceiverInterrupt_backup); }
    }
}

/** ________________________________________________________________________________________________________________________________________________________  */
void RCSwitch::transmitPulse(long HighLow, long duration)
{
    boolean disabled_Receive = false;
    int nReceiverInterrupt_backup = nReceiverInterrupt;
    if (this->nTransmitterPin != -1) 
    {
        if (this->nReceiverInterrupt != -1)  { this->disableReceive();  disabled_Receive = true;}
        
        if (HighLow != 0) HighLow/=HighLow;
        digitalWrite(this->nTransmitterPin, HighLow);
        delayMicroseconds(duration);
        
        if (disabled_Receive){ this->enableReceive(nReceiverInterrupt_backup);}
    }
}

void RCSwitch::setSendZeroEncodingInNbPulse(float high , float low)
{
	zeroEncodingHighLowInNbPulse[0]=high;
	zeroEncodingHighLowInNbPulse[1]=low;
}

void RCSwitch::setSendOneEncodingInNbPulse(float high , float low)
{
	oneEncodingHighLowInNbPulse[0]=high;
	oneEncodingHighLowInNbPulse[1]=low;
}


void RCSwitch::setSendStartLockHighLowInNbPulse(float high , float low)
{
	startLockHighLowInNbPulse[0]=high;
	startLockHighLowInNbPulse[1]=low;
}

void RCSwitch::setSendEndLockHighLowInNbPulse(float high , float low)
{
	endLockHighLowInNbPulse[0]=high;
	endLockHighLowInNbPulse[1]=low;
}


/**  
 * Sends a "0" Bit
 * Waveform Protocol 1: | |___
 * Waveform Protocol 2: | |__
 */
void RCSwitch::send0() 
{
	this->transmit(zeroEncodingHighLowInNbPulse[0],zeroEncodingHighLowInNbPulse[1]);
	/*
	if (this->nProtocol == 1)		{this->transmit(1,3);}
	else if (this->nProtocol == 2) 	{this->transmit(1,2);}
	else if (this->nProtocol == 3) 	{this->transmit(1,5.5f);}
	*/
}

/** ________________________________________________________________________________________________________________________________________________________ 
 * Sends a "1" Bit
 *                       ___  
 * Waveform Protocol 1: |   |_
 *                       __  
 * Waveform Protocol 2: |  |_
 */
void RCSwitch::send1() 
{
	this->transmit(oneEncodingHighLowInNbPulse[0],oneEncodingHighLowInNbPulse[1]);
	/*
	if (this->nProtocol == 1)		{this->transmit(3,1);	}
	else if (this->nProtocol == 2) 	{this->transmit(2,1);}
	else if (this->nProtocol == 3) 	{this->transmit(1,1.26f);}
	*/
}

/** ________________________________________________________________________________________________________________________________________________________ 
 * Sends a "Sync" Bit
 * Waveform Protocol 1: |1|0000000000000000000000000000
 * Waveform Protocol 2: |1|0000000000
 * Waveform Protocol 3: |1|0000000000
 */
void RCSwitch::sendStartSignal() 
{
	this->transmit(startLockHighLowInNbPulse[0],startLockHighLowInNbPulse[1]);
	
	/*
    if (this->nProtocol == 1)			{this->transmit(1,31);}
	else if (this->nProtocol == 2) 		{this->transmit(1,10);}
	else if (this->nProtocol == 3)		
	{	
		transmitPulse(HIGH,200);
		transmitPulse(LOW,2805);
	}
	*/
}

/** ________________________________________________________________________________________________________________________________________________________ 
 * Sends a "Sync" Bit
 * Waveform Protocol 1: |1|0000000000000000000000000000
 * Waveform Protocol 2: |1|0000000000
 * Waveform Protocol 3: |1|0000000000
 */
void RCSwitch::sendEndSignal() 
{
	this->transmit(endLockHighLowInNbPulse[0],endLockHighLowInNbPulse[1]);
}

/** ________________________________________________________________________________________________________________________________________________________ 
 * Enable receiving data
 */
void RCSwitch::enableReceive(int interrupt) 
{
	this->nReceiverInterrupt = interrupt;
	RCSwitch::nReceivedValue = 0;
	RCSwitch::nReceivedBitlength = 0;
	wiringPiISR(this->nReceiverInterrupt, INT_EDGE_BOTH, &handleInterrupt);
}

/** ________________________________________________________________________________________________________________________________________________________ 
 * Disable receiving data
 */
void RCSwitch::disableReceive() 				{this->nReceiverInterrupt = -1;}
/** ________________________________________________________________________________________________________________________________________________________  */
bool RCSwitch::available() 						{return RCSwitch::nReceivedValue != 0;}
/** ________________________________________________________________________________________________________________________________________________________  */
void RCSwitch::resetAvailable() 				{RCSwitch::nReceivedValue = 0;}
/** ________________________________________________________________________________________________________________________________________________________  */
uint64_t RCSwitch::getReceivedValue() 		{return RCSwitch::nReceivedValue;}
/** ________________________________________________________________________________________________________________________________________________________  */
unsigned int RCSwitch::getReceivedBitlength() 	{return RCSwitch::nReceivedBitlength;}
/** ________________________________________________________________________________________________________________________________________________________  */
unsigned int RCSwitch::getReceivedDelay() 		{return RCSwitch::nReceivedDelay;}
/** ________________________________________________________________________________________________________________________________________________________  */
unsigned int RCSwitch::getReceivedProtocol() 	{return RCSwitch::nReceivedProtocol;}
/** ________________________________________________________________________________________________________________________________________________________  */
unsigned int* RCSwitch::getReceivedRawdata() 	{return RCSwitch::timings;}

/** ________________________________________________________________________________________________________________________________________________________  */
bool RCSwitch::processSignal(unsigned int changeCount)
{
    unsigned long delay = RCSwitch::timings[0] / 1;
	uint64_t code = 0;
	if (RCSwitch::timings[changeCount+1] > 30000 ) // si (verrou de fin du signal).low > 30000 alors on considère que le signal est mauvais
	{
		RCSwitch::nReceivedValue 		= code;
		return false;
	}
	RCSwitch::nReceivedBitlength 	= changeCount/2;
	if (RCSwitch::nReceivedBitlength % 8 != 0) // si le nombre de bit du code n'est pas un multiple de 8, idem
	{
		RCSwitch::nReceivedValue 		= code;
		return false;
	}
	
    for (int i = 1; i<changeCount ; i=i+2) 
    {
		if (RCSwitch::timings[i] < 100 || RCSwitch::timings[i+1] < 100) // si c'est le cas, il y a peu de risque que le signal soit valide
		{
			code = 0;
			break;
		}
		if (RCSwitch::timings[i+1] /* low */ > 700)
		{
			code = code << 1;
		}
		else
		{
			code = code << 1;
			code+=1;
		}
	}
	//code = code >> 1;		
    if (changeCount > 6)  // ignore < 4bit values as there are no devices sending 4bit values => noise
    {
      RCSwitch::nReceivedValue 		= code;      
      RCSwitch::nReceivedDelay 		= delay;
	  RCSwitch::nReceivedProtocol 	= 'P';
    }
    return code!=0;
}

/** Function called by a wiringPi thread at each data pin value change */
void RCSwitch::handleInterrupt() 
{
	static unsigned int 	duration;
	static unsigned long 	lastTime;
	static unsigned int 	repeatCount;
	//  static unsigned int changeCount;

	long time = micros();
	duration = time - lastTime;
	
	if (duration > 2750) 
	{    
		if (duration > RCSwitch::timings[0] - 100 /*&& duration < RCSwitch::timings[0] + 200*/)
		{
			repeatCount++;
			changeCount--; // pour ramener changeCount à l'index 0 si début signal dernier bit recu afin d'analyser correctement le signal

			if (repeatCount == 2)  // ça signifie qu'on a recu au moins 1 fois le signal : verrou de début et verrou de fin
			{
				RCSwitch::timings[changeCount+1] = duration;
				if (!processSignal(changeCount))
				{
					//failed
				}
				repeatCount = 0;
			}
		}
		changeCount = 0; // réinitialisation d'une réception
	} 
	else if (changeCount >= RCSWITCH_MAX_CHANGES ) // on a atteint le nombre maximum de changements high-->low
	{
		changeCount = 0;
		repeatCount = 0;
	}
	else if (duration < 50) // peu de chance que ce soit un signal correct
	{
		changeCount = 0;
		repeatCount = 0;
	}
	
	RCSwitch::timings[changeCount++] = duration;	
	lastTime = time;  
}

/** Turns a decimal value to its binary representation */
char* RCSwitch::decTobin(uint64_t dec)
{
	static char bin[256];
	static char binToReturn[256];
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
char* RCSwitch::dec2binWzerofill(uint64_t dec, unsigned int bitLength)
{
	static char paddedBinToReturn[256];

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
