#include "RFTools.h"

using namespace std;

/*------------------------------------------------------------------------------------------------------------------------------*/
/*                                      String operations                                            */
/*------------------------------------------------------------------------------------------------------------------------------*/
// trim from start
static inline string & ltrim(string & s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}
// trim from end
static inline string & rtrim(string & s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}
// trim from both ends
static inline string & trim(string & s) {return ltrim(rtrim(s));}

// toUpper
static std::string toUpper(const std::string & str)
{
	std::locale loc;	
	std::stringstream mystream;
	for (std::string::size_type i=0; i<str.length(); ++i)
		mystream << std::toupper(str[i],loc);
    return mystream.str();
}

//Fonction de conversion int vers string
string intToString(int mylong)
{
	string mystring;
	stringstream mystream;
	mystream << mylong;
	return mystream.str();
}

//Fonction de conversion long vers string
std::string uint64ToString(const uint64_t val)
{   
    std::stringstream mystream;
    mystream << val;
	std::string toReturn = mystream.str();
    mystream.str( std::string() );
	mystream.clear();
    return toReturn;
}

//Fonction de conversion long vers string
std::string longToString(const long mylong)
{   
    std::stringstream mystream;
    mystream << mylong;
	std::string toReturn = mystream.str();
    mystream.str( std::string() );
	mystream.clear();
    return toReturn;
}

//Fonction de conversion long vers string
string doubleToString(double val)
{
    std::stringstream mystream;
    mystream << val;
	std::string toReturn = mystream.str();
    mystream.str( std::string() );
	mystream.clear();
    return toReturn;
}

//Fonction de conversion long vers string
namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}

// Tokenizer
vector<string> tokenize(string & message,const string & sep)
{
	vector<string> toReturn;
	size_t pos 		= 0;
	size_t lastpos 	= 0;
	std::string token;
	pos = message.find(sep.c_str(), lastpos);
	while (pos != string::npos) 
	{
		token = message.substr(lastpos, pos-lastpos);
		toReturn.insert(toReturn.end(),token);
		lastpos = pos + sep.length();
		pos = message.find(sep.c_str(), lastpos+1);
	}
	token = message.substr(lastpos, message.length()-lastpos);
	toReturn.insert(toReturn.end(),token);
	return toReturn;
}

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

	char * decodedBinaryStart = decTobin(dec);
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

/* ----------------------------------------------------------------------------------------------------------- */
int parseCommandLineArgs(int nbArgs, char ** args)
{
	programName = args[0];
	
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
			std:string key = longToString(i);
			commandArgs[key]=args[i];
		}
	}
	
	return 0;
}

/* ----------------------------------------------------------------------------------------------------------- */
void displayCommandLineArgs()
{
	cout << "------------------------------------------------------------------------------------" << endl;
	cout << "------------------------ Command line arguments -------------------" << endl;
	for (std::map<std::string,std::string>::iterator it=commandArgs.begin(); it!=commandArgs.end(); it++)
	{		
    	cout << it->first << " => " << it->second << endl;
	}
	cout << "------------------------------------------------------------------------------------" << endl;
} 

/* ----------------------------------------------------------------------------------------------------------- */
string nowInMicroSecondToString()
{
	struct timeval 	tt;
	gettimeofday(&tt, NULL);
	long t_s = tt.tv_usec/1000L;

	struct tm tstruct;				/* structure date pour les logs */
	time_t timev = time(NULL);
	tstruct = *localtime(&timev);
	char timestr[4096];
	strftime(timestr, sizeof(timestr), "%Y%m%d.%H%M%S", &tstruct);
	char us[4];
	sprintf(us, "%03ld", t_s);
	string totaltimestr = timestr;
	totaltimestr+= ".";
	totaltimestr+= us;
	return totaltimestr;
}

/* ----------------------------------------------------------------------------------------------------------- */
// Fonction de trace
void trace(int level, const char * msg)
{
	if (loglevel <= level)	
	{
		cout << "[" << nowInMicroSecondToString() << "][" << PGRMNAME << "][" << level << "] " << msg << endl;
	}
}

/* ----------------------------------------------------------------------------------------------------------- */
void displayMessage(vector<string> & tokens)
{
	cerr << "[" << nowInMicroSecondToString() << "][" << PGRMNAME << "] ";
	cerr << " ";
	for(int i=0;i < tokens.size();i++)
		cerr << tokens[i] << " ";
	cerr << endl;
}

/* ----------------------------------------------------------------------------------------------------------- */
vector<string> execAndGetOutput(const char * cmd, int & exitCode) 
{
	vector<string> stdoutput;
	FILE * in = popen(cmd, "r");
    //tr1::shared_ptr<FILE> 	pipe(in, pclose);
    if (!pipe) 
    {
		string toTrace = "execAndGetOutput() : error when executing following command : ";
		toTrace += cmd;
		trace(3,toTrace.c_str());
		return stdoutput;
	}
    char buffer[16384];
    string result = "";
    
	while (fgets(buffer, 16383, in) != NULL)
	{
		result = buffer;
		stdoutput.insert(stdoutput.end(),result);
	}
	int stat = pclose(in);
    exitCode = WEXITSTATUS(stat);
	
/*    
    while (!feof(pipe.get())) 
    {
        if (fgets(buffer, 16383, pipe.get()) != NULL)
        {
			result = buffer;
			stdoutput.insert(stdoutput.end(),result);
		}
    }
*/ 
    
    return stdoutput;
}
