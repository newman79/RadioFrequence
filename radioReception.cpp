/*
Cette page récupere les informations du signal radio recu par le raspberry PI et execute une page PHP en lui fournissant tout les paramêtres.

Vous pouvez compiler cette source via la commande :
	g++ radioReception.cpp -o radioReception -lwiringPi
		
	N'oubliez pas d'installer auparavant la librairie wiring pi ainsi que l'essentiel des paquets pour compiler

Vous pouvez lancer le programme via la commande :
	sudo chmod 777 radioReception
	./radioReception /var/www/radioReception/radioReception.php  7

	Les deux parametres de fin étant le chemin vers le PHP a appeller, et le numéro wiringPi du PIN relié au récepteur RF 433 mhz
	
@author : Valentin CARRUESCO (idleman@idleman.fr)
@contributors : Yann PONSARD, Jimmy LALANDE
@webPage : http://blog.idleman.fr
@references & Libraries: https://projects.drogon.net/raspberry-pi/wiringpi/, http://playground.arduino.cc/Code/HomeEasy
@licence : CC by sa (http://creativecommons.org/licenses/by-sa/3.0/fr/)
RadioPi de Valentin CARRUESCO (Idleman) est mis à disposition selon les termes de la 
licence Creative Commons Attribution - Partage dans les Mêmes Conditions 3.0 France.
Les autorisations au-delà du champ de cette licence peuvent être obtenues à idleman@idleman.fr.
*/


#include <wiringPi.h>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>
#include <sstream>

using namespace std;

//initialisation du pin de reception
int pin;

//Fonction de log
void log(string a){
	//Décommenter pour avoir les logs
	cout << a << endl;
}

//Fonction de conversion long vers string
string longToString(long mylong){
    string mystring;
    stringstream mystream;
    mystream << mylong;
    return mystream.str();
}

//Fonction de passage du programme en temps réel (car la reception se joue a la micro seconde près)
void scheduler_realtime() {
	struct sched_param p;
	p.__sched_priority = sched_get_priority_max(SCHED_RR);
	if( sched_setscheduler( 0, SCHED_RR, &p ) == -1 ) {
	perror("Failed to switch to realtime scheduler.");
	}
}

//Fonction de remise du programme en temps standard
void scheduler_standard() {
	struct sched_param p;
	p.__sched_priority = 0;
	if( sched_setscheduler( 0, SCHED_OTHER, &p ) == -1 ) {
	perror("Failed to switch to normal scheduler.");
	}
}

//Recuperation du temp (en micro secondes) d'une pulsation
int pulseInOrig(int pin, int level, int timeout)
{
   struct timeval tn, t0, t1;
   long micros;
   gettimeofday(&t0, NULL);
   micros = 0;
   while (digitalRead(pin) != level)
   {
      gettimeofday(&tn, NULL);
      if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0; // ca signifie qu'au moins 1 seconde s'est passée entre t0 et tn
      micros += (tn.tv_usec - t0.tv_usec);
      if (micros > timeout) return 0;
   }
   gettimeofday(&t1, NULL); // t1 est l'instant ou le signal arrive à level
   while (digitalRead(pin) == level)
   {
      gettimeofday(&tn, NULL);
      if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0; // ca signifie qu'au moins 1 seconde s'est passée entre t0 et tn
      micros = micros + (tn.tv_usec - t0.tv_usec);
      if (micros > timeout) return 0;
   }
   if (tn.tv_sec > t1.tv_sec) micros = 1000000L; else micros = 0; // je ne sais pas pourquoi il fait ça
   micros = micros + (tn.tv_usec - t1.tv_usec);		// ajout différence entre les instants t1 et tn --> durée pendant laquelle level a été appliqué à pin
   return micros; // au final micro seconde est la durée pour une transition [!level-->level] (--> !level)
}

// Recuperation du temp (en micro secondes) d'une pulsation
// 	--> Si pin pas déjà positionné à <level>, attend, depuis l'appel de cette fonction, le moment ou le pin est positionné à <level>
// 	--> Attend que pin soit repositionné à !<level>
//  --> Retourne la durée pendant laquelle le pin est resté à level (en microseconde), ou bien 0 si trop de temps s'est écoulé

struct HighLowPulse
{
	unsigned long high;
	unsigned long low;
};

typedef struct HighLowPulse t_HighLowPulse;

t_HighLowPulse pulseIn(int pin, int level, int timeout)
{
	t_HighLowPulse result;
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
		if (micros > timeout) {	result.low = result.high = 0;	return result;	}
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
		if (result.low > timeout) 	{result.low = result.high = 0; return result;		}
	}
	return result; // au final micro seconde est la durée pour une transition [!level-->level] (--> !level)
}


//Programme principal
int main (int argc, char** argv)
{
	//On passe en temps réel
	scheduler_realtime();
	
	string command;
	string manchesterDecodedSignal;
	
	string path = "php ";
	//on récupere l'argument 1, qui est le chemin vers le fichier php
	path.append(argv[1]);
	log("Demarrage du programme");
	//on récupere l'argument 2, qui est le numéro de Pin GPIO auquel est connecté le recepteur radio
	pin = atoi(argv[2]);
	//Si on ne trouve pas la librairie wiringPI, on arrête l'execution
    if(wiringPiSetup() == -1)
    {
        log("Librairie Wiring PI introuvable, veuillez lier cette librairie...");
        return -1;
    }else{
    	log("Librairie WiringPI detectee");
    }
    pinMode(pin, INPUT);
	log("Pin GPIO configure en entree");
    log("Attente d'un signal du transmetteur ...");
	
	unsigned long signalBitLowTiming[400];
	t_HighLowPulse signalBitPulseTimings[400];
	unsigned long signalbit[400];
	
	//On boucle pour ecouter les signaux
	for(;;)
    {
    	int i = 0;
		//unsigned long t = 0;
	    //avant dernier byte reçu
		int prevBit = 0;
	    //dernier byte reçu
		int bit = 0;
		
		//mise a zero de l'idenfiant télécommande
	    unsigned long sender = 0;
		//mise a zero du groupe
	    bool group=false;
		//mise a zero de l'etat on/off
	    bool on =false;
		//mise a zero de l'idenfiant de la rangée de bouton
	    unsigned long recipient = 0;
		unsigned long premierVerrou = 0;
		unsigned long pulsedelay = 0;
		float delayTolerance = 0.08f;

		command = path+" ";
		
		t_HighLowPulse pulseTimes;
		pulseTimes = pulseIn(pin, LOW, 1000000L);
		
		//Verrou 1 : avec 1700 c'est pas mal
		// 2750 ca marche pas top
		
		// si la durée était < 2,7ms ou > 2.8ms, alors on considère qu'on n'est pas synchronisé avec l'horloge du signal
		while(  !(pulseTimes.low > 13260 && pulseTimes.low < 13350) && !(pulseTimes.low > 11080 && pulseTimes.low < 11130) && !(pulseTimes.low > 2800 && pulseTimes.low < 2820)  )
		{ 
			pulseTimes = pulseIn(pin, LOW,1000000L);
		}
		log("Verrou 1 detecte");	// i.e qu'un HIGH --> LOW --> début d'un HIGH   se produise sur une durée entre 2.7 et 2.8ms 
		premierVerrou = pulseTimes.low;
//		pulsedelay = premierVerrou/31; // On considère dans ce code que c'est le protocole1 (il y a 2 protocoles visiblement 1 où il faut diviser par 31, l'autre par 10 ... je n'en sais pas plus)
		
		cout << " Verrou 1 signal = " << premierVerrou << endl;

/*	
		pulseTimes = pulseIn(pin, LOW, 1000000L);
        if(! (pulseTimes.low > 2800 && pulseTimes.low < 2820))
		{
			cout << " Not a signal because low time of 2nn lock is " << pulseTimes.low << endl;
			i=-1;
		}		
*/
		
		// données
		while(i < 128)
		{
			pulseTimes = pulseIn(pin, LOW, 1000000L);
			//Définition et enregistrement du bit (0 ou 1) et des timings correspondants
	        if(pulseTimes.low > 200 && pulseTimes.low < 700)
			{
				bit = 1;
				signalBitPulseTimings[i].low=pulseTimes.low;
				signalBitPulseTimings[i].high=pulseTimes.high;
				signalBitLowTiming[i]=pulseTimes.low;
				signalbit[i]=bit;
			}
	        else if(pulseTimes.low > 700 && pulseTimes.low < 1700)
			{
				bit = 0;
				signalBitPulseTimings[i].low=pulseTimes.low;
				signalBitPulseTimings[i].high=pulseTimes.high;
				signalBitLowTiming[i]=pulseTimes.low;
				signalbit[i]=bit;
			}
	        else if( (pulseTimes.low > 10700 && pulseTimes.low < 10750) /* normalement c'est la syncro de fin d'un signal de protocol 3 */ || (pulseTimes.low > premierVerrou-100 && pulseTimes.low < premierVerrou+100)  /* normalement c'est la syncro de fin d'un signal de protocol 1 */ ) 
			{
				cout << " Verrou fin message = high: " << pulseTimes.high << " low:" << pulseTimes.low << endl;
				break;
			}
			else
			{
				cout << " Signal aborted at bit number " << i << " because low pulse not in good delay : LastLowPulse =" << pulseTimes.low << endl;
				i = 0;
				break;
			}

/*

Il y a l'air d'avoir 2 verrous : 
   - 1 autour de 3886
   - puis (1 de 150-170us suivi de 0 de 2800 à 2820

Les bits sont ensuite des pulse autour de 250us et 1250us

	10 01 10 10 01 01 10 10 01 01 10 01 10 10 01 10 01 01 01 01 01 01 01 10 01 10 10 01 10 10 10 10


------------------------ TEST2 ----------------------
 Temp verrou = 2808
 Verrou fin message = 2808
 ------- Received signal 1001101001011010010110011010011001010101010101100110100110101001 decimal=1431726505 ------- 
Verrou.LowPulseTime=2808
 b01=( 180, 304) b02=( 255, 1375) b03=( 257, 1364) b04=( 254, 314) b05=( 257, 303) b06=( 246, 1383) b07=( 247, 303) b08=( 254, 1377)
 b09=( 252, 1353) b10=( 248, 317) b11=( 255, 1366) b12=( 255, 313) b13=( 254, 308) b14=( 254, 1370) b15=( 257, 305) b16=( 258, 1378)
 b17=( 261, 1368) b18=( 251, 316) b19=( 250, 1367) b20=( 255, 319) b21=( 246, 308) b22=( 251, 1385) b23=( 245, 1367) b24=( 259, 313)
 b25=( 253, 307) b26=( 252, 1378) b27=( 251, 302) b28=( 257, 1377) b29=( 249, 1365) b30=( 255, 306) b31=( 257, 304) b32=( 257, 1387)
 b33=( 251, 1360) b34=( 256, 321) b35=( 251, 1373) b36=( 251, 315) b37=( 250, 1368) b38=( 250, 312) b39=( 254, 1372) b40=( 252, 317)
 b41=( 255, 1370) b42=( 250, 323) b43=( 245, 1364) b44=( 256, 310) b45=( 257, 1362) b46=( 259, 317) b47=( 255, 302) b48=( 263, 1374)
 b49=( 256, 1364) b50=( 255, 315) b51=( 251, 310) b52=( 249, 1382) b53=( 250, 309) b54=( 251, 1376) b55=( 255, 1368) b56=( 252, 319)
 b57=( 252, 309) b58=( 251, 1377) b59=( 255, 307) b60=( 258, 1373) b61=( 253, 311) b62=( 249, 1372) b63=( 258, 1361) b64=( 256, 307)




------------------------ TEST1 ----------------------
Temp verrou = 3886
 ------- Received signal 0100111111111111111011010010111111111011101100101111111101011110 decimal=4222811998 ------- 
 ------- Received signal 0111111111111110111011010111111111110011010011011111111011110101 decimal=4081975029 ------- 
 ------- Received signal 0111111111110101110110101111111110011101001111011010111101010111 decimal=2638065495 ------- 

Verrou.LowPulseTime=3886
 b01=( 166, 2803) b02=( 257, 304) b03=( 256, 1375) b04=( 258, 1368) b05=( 255, 319) b06=( 251, 308) b07=( 250, 591) b08=(  46, 178)
 b09=(  36, 151) b10=(  58, 154) b11=(  40, 121) b12=( 254, 203) b13=(  22,  81) b14=( 258, 181) b15=(  24, 729) b16=(  39, 409)
 b17=( 242, 149) b18=(  46, 1168) b19=( 259, 317) b20=( 255, 1363) b21=( 257, 318) b22=( 252, 312) b23=( 250, 1377) b24=( 254, 310)
 b25=( 252, 1392) b26=( 251, 1369) b27=( 252, 314) b28=( 260, 1366) b29=( 256, 316) b30=( 254, 307) b31=( 259, 375) b32=(  40, 741)
 b33=(  20, 191) b34=( 255, 722) b35=(  10, 633) b36=( 258, 313) b37=( 258, 306) b38=( 254, 1387) b39=( 242, 311) b40=( 252, 1062)
 b41=(  44, 274) b42=( 250, 1369) b43=( 255, 324) b44=( 246, 314) b45=( 246, 1390) b46=( 254, 1368) b47=( 254, 321) b48=( 246, 1366)
 b49=( 261, 139) b50=(  23, 150) b51=( 260, 620) b52=(  34, 165) b53=(  51, 200) b54=(  18, 275) b55=( 257, 223) b56=(  21,  69)
 b57=( 251, 1375) b58=( 245, 324) b59=( 248, 1374) b60=( 251, 324) b61=( 243, 757) b62=(  24, 590) b63=( 252, 319) b64=( 251, 1371)


Verrou.LowPulseTime=3809
 b01=( 166, 2815) b02=( 251, 314) b03=( 248, 1043) b04=(  54, 167) b05=(  25,  87) b06=( 259, 565) b07=(  55, 183) b08=(  43, 156)
 b09=(  27, 333) b10=( 259, 208) b11=(  25,  79) b12=( 251, 313) b13=( 246, 461) b14=(   7, 916) b15=( 251, 312) b16=( 249, 1387)
 b17=( 243, 700) b18=(  65, 607) b19=( 252, 319) b20=( 252, 1370) b21=( 253, 320) b22=( 248, 315) b23=( 249, 1384) b24=( 249, 313)
 b25=( 248, 1393) b26=( 253, 573) b27=(  38, 130) b28=(  35, 194) b29=(  31, 356) b30=( 265, 206) b31=(  47,  61) b32=( 259, 237)
 b33=(  26, 399) b34=(  34, 671) b35=( 246, 324) b36=( 250, 309) b37=( 251, 1381) b38=( 253, 1373) b39=( 248, 320) b40=( 250, 321)
 b41=( 245, 1372) b42=( 259, 310) b43=( 253, 1380) b44=( 250, 1369) b45=( 252, 320) b46=( 252, 308) b47=( 253, 1392) b48=( 260, 913)
 b49=(  18, 326) b50=(  47,  57) b51=( 252, 316) b52=( 257, 134) b53=(  11, 725) b54=(  72, 428) b55=( 249, 319) b56=( 251, 1379)
 b57=( 246, 328) b58=( 242, 1068) b59=(  30, 274) b60=( 251, 316) b61=( 255, 1372) b62=( 251, 326) b63=( 247, 1369) b64=( 251, 325)

Verrou.LowPulseTime=3876
 b01=( 156, 2818) b02=( 246, 309) b03=( 255, 736) b04=(  67, 335) b05=(  57, 158) b06=( 277, 510) b07=(  30, 148) b08=(  23, 147)
 b09=(  50, 456) b10=( 259, 160) b11=(  13, 144) b12=( 250, 310) b13=( 258, 1380) b14=( 246, 312) b15=( 250, 1384) b16=( 250, 581)
 b17=(  72, 718) b18=( 251, 316) b19=( 252, 1370) b20=( 252, 326) b21=( 247, 311) b22=( 253, 1380) b23=( 249, 315) b24=( 247, 1399)
 b25=( 245, 635) b26=(  50, 333) b27=(  42, 311) b28=( 256, 313) b29=( 257, 479) b30=(  96, 148) b31=(  30, 613) b32=( 252, 318)
 b33=( 255, 312) b34=( 247, 1381) b35=( 248, 1378) b36=( 248, 235) b37=(  32,  55) b38=( 247, 316) b39=( 247, 1383) b40=( 250, 315)
 b41=( 246, 1385) b42=( 250, 1372) b43=( 250, 325) b44=( 244, 315) b45=( 248, 1155) b46=(  52, 159) b47=( 279, 1212) b48=(  24, 137)
 b49=( 245, 320) b50=( 252, 1378) b51=( 246, 325) b52=( 245, 1373) b53=( 253, 325) b54=( 245, 969) b55=(  48, 358) b56=( 245, 318)
 b57=( 255, 1369) b58=( 253, 318) b59=( 252, 1373) b60=( 250, 321) b61=( 248, 1370) b62=( 253, 321) b63=( 252, 308) b64=( 254, 545)
*/
			
			
/* Inutile : ce protocole utilisait le codage Manchester en plus et était sur 32 bits. 1 bit final correspondait alors dans l'algo à une paire de bits décodés selon le pulse time.
			if(i % 2 == 1)
			{
				if((prevBit ^ bit) == 0) // doit être 01 ou 10,,pas 00 ou 11 sinon ou coupe la detection, c'est un parasite
				{
					cout << " signal aborted because of manchester error   i=" << i << endl;
					i = 0;
					break;
				}

				if(i < 53)
				{
					// les 26 premiers (0-25) bits sont l'identifiants de la télécommande
					sender <<= 1;
					sender |= prevBit;
				}      
				else if(i == 53)
				{
					// le 26em bit est le bit de groupe
					group = prevBit;
				}
				else if(i == 55)
				{
					// le 27em bit est le bit d'etat (on/off)
					on = prevBit;
				}
				else
				{
					// les 4 derniers bits (28-32) sont l'identifiant de la rangée de bouton
					recipient <<= 1;
					recipient |= prevBit;
				}
			}
			prevBit = bit;
*/									
			++i;
		} // fin du while
	
// Sélection   OFF
// Group1 BTN1 ON	    	00010101 00010101 01010101										
// Group1 BTN2 ON			00010101 01000101 01010101
// Group1 BTN3 ON			00010101 01010001 01010101
// Group1 BTN4 ON			00010101 01010100 01010101

//	Group1 BTN1 OFF			00010101 00010101 01010100
						
// Group2 BTN1 ON	 		01000101 00010101 01010101								
// Group2 BTN2 ON	 		01000101 01000101 01010101
// Group2 BTN3 ON	 		01000101 01010001 01010101
// Group2 BTN4 ON	 		01000101 01010100 01010101

// Group2 BTN1 OFF			01000101 00010101 01010100
// Group2 BTN2 OFF			01000101 01000101 01010100

// Group3 BTN1 ON	       	01010001 00010101 01010101						
// Group3 BTN1 OFF			01010001 00010101 01010100

// Group4 BTN1 ON	 		01010100 00010101 01010101						
// Group4 BTN1 OFF			01010100 00010101 01010100
//	==> ON/OFF correspond au dernier ou 2 derniers bits
	
		//Si les données ont bien été détéctées
		unsigned long manchesterCodedBit;
		
		if(i>-1)
		{
			cout << " ------- Received signal ";
			unsigned long code = 0;
			for (int k=0;k<i;k++)
			{
				cout << signalbit[k];
				code <<= 1;
				code += signalbit[k];
			}
			if (i==24)
			{
				unsigned long codeGroup = code >> 16;
				unsigned long codeBtn = code >> 8;
				codeBtn = codeBtn - 256* codeGroup;
				unsigned long codeState = code - (256*256)*codeGroup - 256 * codeBtn;
				cout << "   group=" << codeGroup << " btn=" << codeBtn << " state=" << codeState;
			}
			
			if (i==64)
			{
				code = 0;
				manchesterDecodedSignal = "";
				for (int k=0;k<i;k+=2)
				{
					if (signalbit[k] ^ signalbit[k+1] == 0) // doit être 01 ou 10,pas 00 ou 11 sinon ou coupe la detection, c'est un parasite
					{
						manchesterDecodedSignal = " invalid signal";
						break;
					}
					else 
					{
						if (signalbit[k] == 1)				manchesterCodedBit = 1;
						else											manchesterCodedBit = 0;
						manchesterDecodedSignal.append(longToString(manchesterCodedBit));
						code <<= 1;
						code += manchesterCodedBit;
					}
				}
				cout << " manchester code=" << manchesterDecodedSignal << " decimal=" << code;
;
			}			

			
			cout << " ------- " << endl;
			
			cout << "Verrou.LowPulseTime=" << premierVerrou;
			for (int k=0;k<i;k++)
			{
				if (k%8==0)		printf("\n");
				printf(" b%02i=(",(k+1));
				printf("%4i,% 4i)", signalBitPulseTimings[k].high, signalBitPulseTimings[k].low);
			}
			cout << endl;
		
			//on construit la commande qui vas envoyer les parametres au PHP
			command.append(longToString(sender));
			if(group) 	{command.append(" on");}
			else			{command.append(" off");}

			if(on)			{command.append(" on");}
			else			{command.append(" off");}
			
			command.append(" "+longToString(recipient));
			
			//Et hop, on envoie tout ça au PHP
			system(command.c_str());
		}
		else 	{log("Aucune donnee...");	}
		delay(100);
    } // fin boucle for
	
	scheduler_standard();
}
