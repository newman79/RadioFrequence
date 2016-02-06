#ifndef RFTOOLS_H
#define RFTOOLS_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <map>
#include <time.h>
#include <sys/time.h>
#include <vector>
#include <cstdio>
#include <memory>
#include <tr1/memory>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <stdint.h>
#include "json/json.h"

using namespace std;


typedef int (*PtrFncProc)(uint64_t code,unsigned int * timingPtr,unsigned long bitLength);

/*------------------------------------------------------------------------------------------------------------------------------*/
/*                                      String operations                                            */
/*------------------------------------------------------------------------------------------------------------------------------*/
// trim from start
static inline string & 	ltrim(string & s);
// trim from end
static inline string & 	rtrim(string & s);
// trim from both ends
static inline string & 	trim(string & s);
// toUpper
static string 			toUpper(const std::string & str);
//Fonction de conversion int vers string
string 					doubleToString(double mylong);
//Fonction de conversion int vers string
string 					intToString(int mylong);
//Fonction de conversion long vers string
string 					longToString(const long mylong);
//Fonction de conversion long vers string
string		 			uint64ToString(const uint64_t val);

namespace patch
{
    template < typename T > std::string to_string( const T& n );
}

extern string		 					programName;
extern map<string,string>				commandArgs; 								 // arguments de la ligne de commande

// Tokenizer
vector<string> tokenize(string & message,const string & sep);

/** Turns a decimal value to its binary representation */
char* decTobin(uint64_t dec);
char* dec2binWzerofill(uint64_t dec, unsigned int bitLength);

/* ----------------------------------------------------------------------------------------------------------- */
#define COMMANDARGS commandArgs
#define PGRMNAME programName

int parseCommandLineArgs(int nbArgs, char ** args);
/* ----------------------------------------------------------------------------------------------------------- */
void displayCommandLineArgs();
/* ----------------------------------------------------------------------------------------------------------- */
string nowInMicroSecondToString();
/* ----------------------------------------------------------------------------------------------------------- */
extern int loglevel;
// Fonction de trace
void trace(int level, const char * msg);
/* ----------------------------------------------------------------------------------------------------------- */
void displayMessage(vector<string> & tokens);
/* ----------------------------------------------------------------------------------------------------------- */
vector<string> execAndGetOutput(const char * cmd, int & exitCode);

#endif
