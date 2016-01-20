/*
  RF_Sniffer
  
  by @justy to provide a handy RF code sniffer
*/

#include "RCSwitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
       
RCSwitch mySwitch;
 
int main(int argc, char *argv[]) {
  
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
			int value = mySwitch.getReceivedValue();
    
			if (value == 0) 
			{
				printf("Unknown encoding\n");
			} 
    	    else 
			{   
				unsigned long receivedValue = mySwitch.getReceivedValue(); 
				unsigned long bitLength = mySwitch.getReceivedBitlength();
				unsigned long delay = mySwitch.getReceivedDelay();
				unsigned int protocol = mySwitch.getReceivedProtocol();
				
				unsigned int * timingPtr = mySwitch.getReceivedRawdata();
				
				printf("------ Received signal :  %i bits  -    SyncLength=%ius,  SignalPulseDelay(=SyncLength/3 in protocol1 SyncLength/2 in protocol 2)=%ius       Protocol=%i  ------ \n",  bitLength,  timingPtr[0] , delay,  protocol);
				printf(" value=%i  Ox%x   %s \n",receivedValue,receivedValue, RCSwitch::dec2binWzerofill(receivedValue,24));
								
				int timingsSize = 2 * bitLength;
				for (int i=1;i<timingsSize+1;i++)
				{
					if (i%2==1)
					{
						printf(" b%02i.times=",(i+1)/2);
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
