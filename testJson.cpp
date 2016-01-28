#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream> 

// This is the JSON header
#include "json/json.h"

using namespace std;

int main(int argc, char **argv)
{
	string json_example = "{\"array\": \
							[\"item1\", \
							\"item2\"], \
							\"not an array\": \
							\"asdf\" \
						 }";
	ifstream jsonFile;
	jsonFile.open("/home/pi/src/RadioFrequence/radioFrequenceSignalConfig.json");

	// Let's parse it  
	Json::Value 	root;
	Json::Reader 	reader;
		
	//bool parsedSuccess = reader.parse(json_example, root, false);
	bool parsedSuccess = reader.parse(jsonFile, root, false);
	jsonFile.close();

	if(not parsedSuccess) // Report failures and their locations in the document.
	{
		cout<<"Failed to parse JSON"<< endl << reader.getFormatedErrorMessages() << endl;
		return 1;
	}

/*
	// Let's extract the array contained in the root object
	const Json::Value array = root["array"];

	// Iterate over sequence elements and print its values
	for(unsigned int index=0; index<array.size(); ++index)  
	{  
		cout<<"Element " << index <<" in array: " << array[index].asString() << endl;
	}

	// Lets extract the not array element contained in the root object and print its value
	const Json::Value notAnArray = root["not an array"];

	if(not notAnArray.isNull())
	{
		cout<<"Not an array: " << notAnArray.asString() << endl;
	}
*/
	// If we want to print JSON is as easy as doing:
	cout<<"Json Example pretty print: " << endl << root.toStyledString() << endl;


	const Json::Value protocole1 = root["protocole1"];
	if(not protocole1.isNull())
	{
		const Json::Value protocole1_endlock_highlow__NbPulseLength = protocole1["OneEncoding_highlow_NbPulseLength"];
		if (not protocole1_endlock_highlow__NbPulseLength.isNull() && protocole1_endlock_highlow__NbPulseLength.isArray())
		{
			for(unsigned int index=0; index<protocole1_endlock_highlow__NbPulseLength.size(); ++index)  
			{  
				cout<<"Element " << index <<" of OneEncoding_highlow_NbPulseLength : " << protocole1_endlock_highlow__NbPulseLength[index].asString() << endl;
			}
		}
	}


	return 0;
}
