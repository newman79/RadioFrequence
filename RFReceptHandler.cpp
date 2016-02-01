/*
  RF_Sniffer
  
  by @justy to provide a handy RF code sniffer
*/

#include "RCSwitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
       
RCSwitch mySwitch;
 
int main(int argc, char *argv[]) 
{
  
     // This pin is not the first pin on the RPi GPIO header!
     // Consult https://projects.drogon.net/raspberry-pi/wiringpi/pins/
     // for more information.
     int PIN = 2;
     
	if(wiringPiSetup() == -1)
		return 0;

	mySwitch = RCSwitch();
	mySwitch.enableReceive(PIN);  // Receiver on inerrupt 0 => that is pin #2

	while(1)  
	{ 
		if (mySwitch.available()) 
		{
			uint64_t value = mySwitch.getReceivedValue();
    
			if (value == 0) 
			{
				printf("Unknown encoding\n");
			} 
    	    else 
			{
				printf ( "CRFReceptHandler.getpid = %d\n", getpid());				
				//std::cout << "RFReceptHandler.getpid=" << getpid << std::endl;
				
				unsigned long bitLength 	= mySwitch.getReceivedBitlength();
				unsigned long delay 		= mySwitch.getReceivedDelay();
				unsigned int protocol 		= mySwitch.getReceivedProtocol();
				
				unsigned int * timingPtr 	= mySwitch.getReceivedRawdata();
				
				int timingsSize = 2 * bitLength;
				printf("------------------------------------------------------------------------------------------- \n");
				printf("Received signal code :  %lu bits  \n",  bitLength);
				printf("  value=%llu  %s \n",value, RCSwitch::dec2binWzerofill(value,bitLength));
				printf("  Pulses : StartLock.low= %ius        EndLock.high=%ius EndLock.low=%ius \n", timingPtr[0],  timingPtr[timingsSize+1],timingPtr[timingsSize+2]);
								
				for (int i=1;i<timingsSize+1;i++)
				{
					if (i%2==1)
					{
						printf("     b%02i=",(i+1)/2);
					}
					printf("% 4i ", timingPtr[i]);
					
					if (i%16==0)
					printf("\n");
				}
			}
			printf("\n");
			mySwitch.resetAvailable();   
			usleep(10000);
		}
  	}

	exit(0);
}
