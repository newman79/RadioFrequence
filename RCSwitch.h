/*
  RCSwitch - Arduino libary for remote control outlet switches
  Copyright (c) 2011 Suat Özgür.  All right reserved.

  Contributors:
  - Andre Koehler / info(at)tomate-online(dot)de
  - Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
  
  Project home: http://code.google.com/p/rc-switch/

  This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef _RCSwitch_h
#define _RCSwitch_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#else
    #include <wiringPi.h>
    #include <stdint.h>
    #define NULL 0
    #define CHANGE 1
#ifdef __cplusplus
extern "C"{
#endif
typedef uint8_t boolean;
typedef uint8_t byte;

#if !defined(NULL)
#endif
#ifdef __cplusplus
}
#endif
#endif

#include <iostream>
#include <sstream>

namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}


// Number of maximum High/Low changes per packet.
// We can handle up to (unsigned long) => 32 bit * 2 H/L changes per bit + 2 for sync
#define RCSWITCH_MAX_CHANGES 140


class RCSwitch 
{
  public:
    RCSwitch();
    
    void send(unsigned long Code, unsigned int length);
    void send(char* Code);
    std::string computeAndDisplaySignalToSend(char* Code);
    
    void enableReceive(int interrupt);
    void disableReceive();
    bool available();
	void resetAvailable();
	
    uint64_t 	getReceivedValue();
    unsigned int getReceivedBitlength();
    unsigned int getReceivedDelay();
	unsigned int getReceivedProtocol();
    unsigned int* getReceivedRawdata();
  
    void enableTransmit(int nTransmitterPin);
    void disableTransmit();
    void setPulseLength(int nPulseLength);
    void setRepeatTransmit(int nRepeatTransmit);
    void setReceiveTolerance(int nPercent);
	void setProtocol(int nProtocol);
	void setProtocol(int nProtocol, int nPulseLength);
	void setSendZeroEncodingInNbPulse(float high , float low);
	void setSendOneEncodingInNbPulse(float high , float low);
	void setSendStartLockHighLowInNbPulse(float high , float low);
	void setSendEndLockHighLowInNbPulse(float high , float low);

    static char* dec2binWzerofill(uint64_t dec, unsigned int length);
    static char* decTobin(uint64_t dec);
	  
  private:
    void send0();
    void send1();
    void sendStartSignal();
    void sendEndSignal();
    void transmit(float nHighPulses, float nLowPulses);
    void transmitPulse(long HighLow, long duration);
    
    static void handleInterrupt();
	static bool processSignal(unsigned int changeCount);

    int 	nReceiverInterrupt;		/* pin gpio data récepteur */
    int 	nTransmitterPin;		/* pin gpio data émetteur */
    
    int 	nPulseLength; 			/* Durée minimale d'un élément de signal (high ou low).  Pour protocoles 1 et 2, Durée d'un high= Nombre * nPulseLength */
    int 	nRepeatTransmit;		/* Nombre de répétition du signal à transmettre */
	char 	nProtocol;				/* Protocole défini pour un signal à envoyer */

	float 	zeroEncodingHighLowInNbPulse[2];  	/* 0 sera encodé par HIGH sur zeroEncodingHighLowInNbPulse[0]*pulseLength puis LOW sur zeroEncodingHighLowInNbPulse[1]*pulseLength */
	float 	oneEncodingHighLowInNbPulse[2]; 	/* 1 sera encoté par ... */
	float 	startLockHighLowInNbPulse[2];  	
	float 	endLockHighLowInNbPulse[2]; 	


	static int 				nReceiveTolerance;				/* Tolérance pour le traduction des durée des pulses en bit */
    static uint64_t		 	nReceivedValue;					/* Code de signal recu */
    static unsigned int 	nReceivedBitlength; 			/* Nombre de bits du code de signal recu */
	static unsigned int 	nReceivedDelay;					/* BitLow du verrou de début du signal */
	static unsigned int 	nReceivedProtocol;				/* Protocol du signal recu */
    static unsigned int 	timings[RCSWITCH_MAX_CHANGES];	/* Toutes les pulsations recues */
	static unsigned int 	changeCount;					/* Nombre de transition HIGH-->LOW ou LOW-->HIGH */
};

#endif
