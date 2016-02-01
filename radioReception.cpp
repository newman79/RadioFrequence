/* Récupere les informations du signal radio recu par le raspberry PI et execute une page PHP en lui fournissant tout les paramêtres.

Prérequis : Installer la librairie wiring pi ainsi que l'essentiel des paquets pour compiler

Vous pouvez compiler cette source via la commande :
	g++ radioReception.cpp -o radioReception -lwiringPi	

Lancer le programme via la commande :
	sudo chmod 777 radioReception
	./radioReception <Chemin_Vers_radioReception.php>  <pinGPIO_relieAuRecepteurRadioFrequence>

@authors : Xavier MARQUIS
@orignal authors : Valentin CARRUESCO (idleman@idleman.fr)
@contributors : Yann PONSARD, Jimmy LALANDE
@webPage : http://blog.idleman.fr
@references & Libraries: https://projects.drogon.net/raspberry-pi/wiringpi/, http://playground.arduino.cc/Code/HomeEasy
@licence : CC by sa (http://creativecommons.org/licenses/by-sa/3.0/fr/)
*/

#include <wiringPi.h>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>
#include <sstream>
#include <map>
#include <unistd.h>
#include <stdint.h>

using namespace std;

struct HighLowPulse
{
	unsigned long high;
	unsigned long low;
};

typedef struct HighLowPulse t_HighLowPulse;

int 							loglevel=0;				/* pinGPIO relié à la sortie data du récepteur RadioFrequence */
stringstream 					otrace;

int 							pin; 					/* pinGPIO relié à la sortie data du récepteur RadioFrequence */
time_t  						timev;					/* Date pour les logs */
char 							timestr[64];			/* string date pour les logs */
struct tm  						tstruct;				/* structure date pour les logs */
string							progname;				/* name of the program */
map<std::string,std::string>	commandArgs; 			/* arguments de la ligne de commande */

t_HighLowPulse 					pulseTimes;				/*  Pulsation captée (elle est gardée temporairement) */

string 							manchesterDecodedSignal;/* signal */
t_HighLowPulse verrouFinSignal;							/* signal */
t_HighLowPulse verrouDebutSignal;						/* signal */
t_HighLowPulse signalBitPulseTimings[400];				/* signal */
unsigned long 	signalbit[400];							/* signal */

string pgrmToCall;										/* program to call with received siganl */

//Fonction de conversion long vers string
string longToString(const long mylong)
{   
    stringstream mystream;
    mystream << mylong;
	string toReturn = mystream.str();
    mystream.str( std::string() );
	mystream.clear();
    return toReturn;
}

//Fonction de conversion long vers string
string intToString(const int myint)
{
    stringstream mystream;
    mystream << myint;
	string toReturn = mystream.str();
    mystream.str( std::string() );
	mystream.clear();
    return toReturn;
}

//Fonction de log
void log(char * msg,int level)
{
	if (loglevel <= level)
	{
		time(&timev);
		tstruct = *localtime(&timev);
		strftime(timestr, sizeof(timestr), "%Y%m%d_%H%M%S", &tstruct);
		cout << " [ " << timestr << " " << progname << " ] " << msg << endl;
	}
}
// Fonction de trace
void trace(stringstream & mystream,int level)
{
	if (loglevel <= level)	{cerr << mystream.str();}
	mystream.str( std::string() );
	mystream.clear();
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
			if (key.length() == 0)
				key=value;
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
	for (std::map<std::string,std::string>::iterator it=commandArgs.begin(); it!=commandArgs.end(); )
	{		
    	cout << it->first << " => " << it->second << endl;
	}
	cout << "------------------------------------------------------------------------------------" << endl;
} 

//Fonction de passage du programme en temps réel (car la reception se joue a la micro seconde près)
void scheduler_realtime() 
{
	log("Start of scheduler_realtime",0);	
	struct sched_param p;
	p.__sched_priority = sched_get_priority_max(SCHED_RR);
	if( sched_setscheduler( 0, SCHED_RR, &p ) == -1 ) {perror("Failed to switch to realtime scheduler.");}
}

//Fonction de remise du programme en temps standard
void scheduler_standard() 
{
	log("Start of scheduler_standard",0);
	struct sched_param p;
	p.__sched_priority = 0;
	if( sched_setscheduler( 0, SCHED_OTHER, &p ) == -1 ) {perror("Failed to switch to normal scheduler.");}
}

// Recuperation du temp (en micro secondes) d'une pulsation <!level> --> <level> --> <!level>.début
// 	--> Si pin pas déjà positionné à <level>, attend, depuis l'appel de cette fonction, le moment ou le pin est positionné à <level>
// 	--> Attend que pin soit repositionné à !<level>
//  --> Retourne les durées de transitions dans un object t_HighLowPulse , ou bien un object t_HighLowPulse dont l'attribut high vaut 0 si plus de <timeout> us se sont écoulées.
void pulseIn(int pin, int level, int timeout, t_HighLowPulse & result)
{
	result.high=0;
	result.low=0;

	struct timeval 	tn, 	t0, 	t1;  		// t0 = début d'appel de la fonction, t1 = instant ou 
	long 			tn_us,	t0_us,	t1_us;	// t<i>_us est égal aux variables t0, t1, tn en les convertissant en microseconde : t<i>_us = t<i>.tv_sec * 1000000L + t<i>.tv_usec
	long micros;
	gettimeofday(&t0, NULL);
	t0_us = t0.tv_sec * 1000000L + t0.tv_usec;
	micros = 0;
	// attend jusqu'à ce que pin soit positionné à <level>
	while (digitalRead(pin) != level)
	{
		gettimeofday(&tn, NULL);
		tn_us = tn.tv_sec * 1000000L + tn.tv_usec;
		micros = tn_us - t0_us;
		if (micros > timeout) {	result.low = result.high = 0;	return;	}
	}
	gettimeofday(&t1, NULL); // t1 est l'instant ou le signal arrive à level
	t1_us = t1.tv_sec * 1000000L + t1.tv_usec;
	result.high = t1_us - t0_us;
	
	// attend jusqu'à ce que pin soit positionné à !<level>
	while (digitalRead(pin) == level)
	{
		gettimeofday(&tn, NULL);
		tn_us = tn.tv_sec * 1000000L + tn.tv_usec;
		result.low = tn_us - t1_us;
		if (result.low > timeout) 	{result.low = result.high = 0; return;		}
	}
	return; // au final micro seconde est la durée pour une transition [!level-->level] (--> !level)
}

//---------------------------------------------------------------------------------- Programme principal ------------------------------------------------------------------------------------------------//
int main (int argc, char** argv)
{
	//progname = argv[0];
	progname = "RFReceptHandler";
	parseCommandLineArgs(argc, argv);
	displayCommandLineArgs();
	log("---------- Taper CTRL^C pour quitter --------",0);
		
	//scheduler_realtime(); //On passe en temps réel
	
	string path = "php ";
	//on récupere l'argument 1, qui est le chemin vers le fichier php
	log("Demarrage du programme",0);
	//on récupere l'argument 2, qui est le numéro de Pin GPIO auquel est connecté le recepteur radio
	
	pin = -1;
	std::map<std::string,std::string>::iterator it;
	it = commandArgs.find("pin");
	if(it != commandArgs.end())		pin=atoi(commandArgs["pin"].c_str());
	else
	{
		log("syntax : RFReceptHandler -pin=<pinNumber> -call=<pgrmToCallWhenReceivedRFSignal>",3);
		return 1;
	}
	it = commandArgs.find("call");
	if(it != commandArgs.end())		pgrmToCall=commandArgs["call"];
	else
	{
		log("syntax : RFReceptHandler -pin=<pinNumber> -call=<pgrmToCallWhenReceivedRFSignal>",3);
		return 2;
	}
	it = commandArgs.find("s");
	if(it != commandArgs.end())	{loglevel=2;}

	//Si on ne trouve pas la librairie wiringPI, on arrête l'execution
    if(wiringPiSetup() == -1)  	{log("Librairie Wiring PI introuvable, veuillez lier cette librairie...",0); return -1;}
    else 						{log("Librairie WiringPI detectee",0); }
    pinMode(pin, INPUT);
	log("Pin GPIO configure en entree",0);
    log("Attente d'un signal du transmetteur ...",0);
	
	
	//On boucle pour ecouter les signaux
	for(;;)
    {
    	int i = 0;
	    //avant dernier byte reçu 
		//int prevBit = 0;
	    //dernier byte reçu
		int bit = 0;
		
		verrouFinSignal.low = 0;
		verrouFinSignal.high = 0;
		unsigned long pulsedelay = 0;
		//float delayTolerance = 0.08f;

		pulseIn(pin, LOW, 50000L,pulseTimes);		
		// si la durée était < 2,7ms ou > 2.8ms, alors on considère qu'on n'est pas synchronisé avec l'horloge du signal
		bool loop = true;
		while (loop)
		{
			delay(300);			
			for(unsigned long cpt=0;cpt<1000;cpt++)
			{
				pulseIn(pin, LOW,50000L, pulseTimes);
				if ( (pulseTimes.low > 13240 && pulseTimes.low < 13350) || (pulseTimes.low > 10800 && pulseTimes.low < 11280) || (pulseTimes.low > 2800 && pulseTimes.low < 2820) )
				{
					loop=false;
					break;
				}
			}
			//	while(  !(pulseTimes.low > 13240 && pulseTimes.low < 13350) && !(pulseTimes.low > 11140 && pulseTimes.low < 11280) && !(pulseTimes.low > 2800 && pulseTimes.low < 2820)  )
			//	{
			//		pulseIn(pin, LOW,50000L, pulseTimes);
			//	}
		}
		verrouDebutSignal.low = pulseTimes.low;
		pulsedelay = verrouDebutSignal.low/31; // On considère dans ce code que c'est le protocole1 (il y a 2 protocoles visiblement 1 où il faut diviser par 31, l'autre par 10 ... je n'en sais pas plus)

		// Récupération du signal proprement dit : ici, on récupère les bits du signal
		while(i < 128)
		{
			pulseIn(pin, LOW, 50000L, pulseTimes);
			//Définition et enregistrement du bit (0 ou 1) et des timings correspondants
	        if(pulseTimes.low > 200 && pulseTimes.low < 700)
			{
				bit = 1;
				signalBitPulseTimings[i].low=pulseTimes.low;
				signalBitPulseTimings[i].high=pulseTimes.high;
				signalbit[i]=bit;
			}
	        else if(pulseTimes.low > 700 && pulseTimes.low < 1700)
			{
				bit = 0;
				signalBitPulseTimings[i].low=pulseTimes.low;
				signalBitPulseTimings[i].high=pulseTimes.high;
				signalbit[i]=bit;
			}
	        else if( (pulseTimes.low > 10700 && pulseTimes.low < 10800) /* normalement c'est la syncro de fin d'un signal de protocol 3 */ || (pulseTimes.low > verrouDebutSignal.low-100 && pulseTimes.low < verrouDebutSignal.low+100)  /* normalement c'est la syncro de fin d'un signal de protocol 1 */ ) 
			{
				verrouFinSignal.high	= pulseTimes.high;
				verrouFinSignal.low 	= pulseTimes.low;
				break;
			}
			else
			{
				
				otrace << " Signal aborted at bit number " << i << " because low pulse not a signal bit delay registered in protocol database : LastLowPulse =" << pulseTimes.low << endl;
				trace(otrace, 1);
				i = 0;
				break;
			}
				/* Inutile : ce protocole utilisait le codage Manchester en plus et était sur 32 bits. 1 bit final correspondait alors dans l'algo à une paire de bits décodés selon le pulse time.
				if(i % 2 == 1)
				{
					if((prevBit ^ bit) == 0) // doit être 01 ou 10,,pas 00 ou 11 sinon ou coupe la detection, c'est un parasite
					{
						cout << " signal aborted because of manchester error   i=" << i << endl;
						i = 0;
						break;
					}

					if(i < 53)						{	sender <<= 1;	sender |= prevBit; 	}  // les 26 premiers (0-25) bits sont l'identifiants de la télécommande    
					else if(i == 53)			{	group = prevBit;	} // le 26em bit est le bit de groupe
					else if(i == 55) 			{	on = prevBit; } // le 27em bit est le bit d'etat (on/off)
					else								{	recipient <<= 1;	recipient |= prevBit;} // les 4 derniers bits (28-32) sont l'identifiant de la rangée de bouton
				}
				prevBit = bit;
				*/									
			++i;
		} // fin du while
		
		//Si 128 bits atteints ou si verrou de fin de signal recu
		otrace << "----------------------------------------------------------------------------------------------------" << endl;
		trace(otrace, 2);
		if(i>0)
		{
			time(&timev);
			otrace << "At " << timev << " : Received new signal rawcode=";
			unsigned long code = 0;
			for (int k=0;k<i;k++)
			{
				otrace << signalbit[k];
				code <<= 1;
				code += signalbit[k];
			}
			if (i==24)
			{
				unsigned long codeGroup = code >> 16;
				unsigned long codeBtn = code >> 8;
				codeBtn = codeBtn - 256* codeGroup;
				unsigned long codeState = code - (256*256)*codeGroup - 256 * codeBtn;
				otrace << "   decoded :  group=" << codeGroup << " btn=" << codeBtn << " state=" << codeState;
				trace(otrace, 2);
			}
			else if (i==64) // signal de 64 bit --> on considère que c'est un signal codé en manchester
			{
				unsigned long manchesterCodedBit;				
				code = 0;
				manchesterDecodedSignal = "";
				uint64_t rawdecimal=0;
				
				for (int k=0;k<i;k++)
				{
					rawdecimal <<= 1;
					if (signalbit[k]==1)
						rawdecimal += 1ll;
				}
				
				
				for (int k=0;k<i;k+=2)
				{
					if (signalbit[k] ^ signalbit[k+1] == 0)  /* doit être 01 ou 10,pas 00 ou 11 sinon ou coupe la detection, c'est un parasite */ 
					{
						manchesterDecodedSignal = " not a manchester signal";
						break;
					}
					else 
					{
						if (signalbit[k] == 1)				manchesterCodedBit = 1;
						else								manchesterCodedBit = 0;
						manchesterDecodedSignal.append(longToString(manchesterCodedBit));
						code <<= 1;
						code += manchesterCodedBit;
					}
				}
				otrace << " decoded : manchester code=" << manchesterDecodedSignal << " decimal=" << code << " rawdecimal=" << rawdecimal << endl;
				
				
				trace(otrace, 2);
			}
			else
			{
				otrace << " decoded : unknown signal (" << i << "bits ) " << endl;
				trace(otrace, 2);
			}
			// Affichage de toutes les pulsations du signal
			otrace << endl << " Signal pulses : " << endl;
			otrace << "    VerrouDebutSignal [ high,low ]=" << verrouDebutSignal.high 	<< "," << verrouFinSignal.low;
			otrace << "               VerrouFinSignal [ high,low ]=" << verrouFinSignal.high 		<< "," << verrouFinSignal.low;			
			otrace << "    pulsedelay=" << pulsedelay;
			char strToTrace[2048];
			for (int k=0;k<i;k++)
			{
				if (k%8==0)	{	sprintf(strToTrace,"\n   ");otrace << strToTrace; }
				sprintf(strToTrace," b%02i=(",(k+1));
				otrace << strToTrace;
				sprintf(strToTrace,"%4i,% 4i)", signalBitPulseTimings[k].high, signalBitPulseTimings[k].low);
				otrace << strToTrace;
			}			
			otrace << endl;
			trace(otrace, 1);
		
			otrace << endl << " handled program call ... ";
			trace(otrace, 2);
			//Et hop, on envoie tout ça au PHP
			system(pgrmToCall.c_str());
		}
		else 	
		{
			otrace << "Invalid signal ..." << endl;	
			otrace << "    VerrouDebutSignal [ high,low ]=" << verrouDebutSignal.high 	<< "," << verrouFinSignal.low << endl;
			trace(otrace, 1);
		}
		delay(200);
    } // fin boucle for
	 
	scheduler_standard();
}
