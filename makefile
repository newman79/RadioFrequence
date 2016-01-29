CXXFLAGS=-Wwrite-strings -Wformat -Wparentheses

all:  radioReception  RFReceptHandler  RFSend testJson RFSendRemoteBtnCode

RFSend: RCSwitch.o jsoncpp.o RFSend.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@ -lwiringPi

RFSendRemoteBtnCode: RCSwitch.o jsoncpp.o RFSendRemoteBtnCode.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@ -lwiringPi
	
RFReceptHandler: RCSwitch.o RFReceptHandler.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@ -lwiringPi
	
radioReception: radioReception.o
	$(CXX) -Wwrite-strings -Wformat -Wparentheses $(LDFLAGS) $+ -o $@ -lwiringPi

testJson: jsoncpp.o testJson.cpp
	$(CXX) $(CXXFLAGS) -Wwrite-strings -Wformat -Wparentheses $(LDFLAGS) $+ -o $@ 

clean:
	$(RM) *.o  RFSend radioReception RFReceptHandler
