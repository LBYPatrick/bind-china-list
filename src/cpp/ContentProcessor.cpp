#include <string>
#include <fstream>
#include "base64.h"
#include "ContentProcessor.h"


using std::string;

	void ContentProcessor::init(string & rawContent, int inputType, int outputType) {
		sourceType = inputType;
		targetType = outputType;
		rawFileContent = (inputType == 0) ? rawContent : base64::decode(rawContent);
	}

	void ContentProcessor::getRawList() {

		if (sourceType == 1) rawFileContent = base64::decode(rawFileContent);
		readBuffer = ""; //cleanup

		for (int i = 0; i < (int)rawFileContent.size(); ++i) {

			readBuffer += rawFileContent[i];
			
			if (rawFileContent[i+1] == '\n') {	
					
				if (sourceType == 0) { //dnsmasq
					dnsListCache += dnsmasq.getRawDNS(readBuffer) + '\n';
					domainListCache += dnsmasq.getRawDomain(readBuffer) + '\n';
					//printf("%s\n",dnsmasq.getRawDomain(readBuffer).c_str());
					//system("pause");
				}
				else if (sourceType == 1) { //GFWList
					if (!(gfwlist.getRawDomain(readBuffer) == "")) {
						domainListCache += gfwlist.getRawDomain(readBuffer) + "\n";
					}
				}
				readBuffer = "";
				i += 1;
			}
		}
	}

	void ContentProcessor::convert() {

		
			  int			rotatorOne			= 0;
			  int			rotatorTwo			= 0;
			  string		readDNSBuffer		= "";
			  string		readDomainBuffer	= "";
		const bool			isDnsmasqAndBind	= (sourceType == 0 && targetType == 1);
			  bool			isFirstLine			= true;
			  bool			isDNSCustomed		= customDNS != "";
			  bool			isProxyCustomed		= customProxy != "";

		readBuffer = "";

		//Header
		switch (targetType) {
		case 0: //Shadowrocket
			outputContent += "[General]\n\n";
			outputContent += "bypass-system = true";
			outputContent += "\nskip - proxy = 192.168.0.0 / 16, 10.0.0.0 / 8, 172.16.0.0 / 12, localhost, *.local, e.crashlynatics.com, captive.apple.com";
			outputContent += "\nbypass-tun = 10.0.0.0/8,100.64.0.0/10,127.0.0.0/8,169.254.0.0/16,172.16.0.0/12,192.0.0.0/24,192.0.2.0/24,192.88.99.0/24,192.168.0.0/16,198.18.0.0/15,198.51.100.0/24,203.0.113.0/24,224.0.0.0/4,255.255.255.255/32";
			outputContent += R"(skip - proxy = 192.168.0.0 / 16, 10.0.0.0 / 8, 172.16.0.0 / 12, localhost, *.local, e.crashlynatics.com, captive.apple.com)";
			outputContent += "\n";
			outputContent += R"(bypass-tun = 10.0.0.0/8,100.64.0.0/10,127.0.0.0/8,169.254.0.0/16,172.16.0.0/12,192.0.0.0/24,192.0.2.0/24,192.88.99.0/24,192.168.0.0/16,198.18.0.0/15,198.51.100.0/24,203.0.113.0/24,224.0.0.0/4,255.255.255.255/32)";
			outputContent += "\n\n\n\n";
			outputContent += R"([Rule])";
			outputContent += "\n";
			break;
		case 1:break;
		case 2:
		case 3:
			outputContent += "var domains = {\n";
			break;
		}

		//Read and process output content line by line 
		while (rotatorTwo < domainListCache.size()) {
			
			if (isDnsmasqAndBind) { //Get DNS if sourceType is DNSMASQ and targetType is bind
				while(true) { //Get DNS
					readDNSBuffer += dnsListCache[rotatorOne];
					if (dnsListCache[rotatorOne + 1] == '\n') { rotatorOne += 2; break; }
					rotatorOne++;
				}
			}
			
			while(true) { //Get Domain
				readDomainBuffer += domainListCache[rotatorTwo];
				if (domainListCache[rotatorTwo + 1] == '\n') { rotatorTwo += 2; break; }
				rotatorTwo++;
			}

			//Format
			switch (targetType) {
				case 0 : 
					outputContent += "\tDOMAIN-KEYWORD,";
					outputContent += readDomainBuffer;
					outputContent += ",Direct\n";
					break;
				
				case 1 :
					outputContent += "zone \"";
					outputContent += readDomainBuffer;
					outputContent += "\" {\n\t type forward;\n\t forwarders {";
					
					if (isDnsmasqAndBind) { 
						if (isDNSCustomed) outputContent += customDNS;
						else			   outputContent += readDNSBuffer; 
					}
					else				   outputContent += customDNS;

					outputContent += ";};\n};\n\n";
					break;
				
				case 2 :
				case 3 :
					if (!isFirstLine) outputContent += ",\n";
					else			  isFirstLine = false;

					outputContent += "\t\"";
					outputContent += readDomainBuffer;
					outputContent += "\" : 1";
					break;
			}
			readDNSBuffer = "";
			readDomainBuffer = "";
		}

		//End Message
		switch (targetType) {
		
		case 0 : outputContent += "\n\n\nFINAL,Proxy\n"; break;
		case 1 : break;
		case 2 : 
		case 3 :
			outputContent += "\n\n};\n\n\n";
			outputContent += "var proxy = \"";
			outputContent += (targetType == 2)? "__PROXY__" : (isProxyCustomed? customProxy : defaultProxy);
			outputContent += "\";\n";
			outputContent += "var direct = 'DIRECT;';\n";
			outputContent += "var hasOwnProperty = Object.hasOwnProperty;\n\n";
			outputContent += "function FindProxyForURL(url, host) {\n\n";
			outputContent += "\tvar suffix;\n";
			outputContent += "\tvar pos = host.lastIndexOf('.');\n";
			outputContent += "\tpos = host.lastIndexOf('.', pos - 1);\n\n";
			outputContent += "\twhile(1) {\n";
			outputContent += "\t\tif (pos <= 0) {\n";
			outputContent += "\t\t\tif (hasOwnProperty.call(domains, host)) return direct;\n";
			outputContent += "\t\t\telse return proxy;\n\t\t}\n\n";
			outputContent += "\tsuffix = host.substring(pos + 1);\n";
			outputContent += "\tif (hasOwnProperty.call(domains, suffix)) return direct;\n";
			outputContent += "\tpos = host.lastIndexOf('.', pos - 1);\n\t}\n}";
		}
	}

	string inline ContentProcessor::dnsmasqProcessor::getRawDomain(string originLine) {
		string returnBuf = "";
		for (int counter = 8; counter < (int)originLine.find_last_of("/"); counter++) returnBuf += originLine[counter];
		return returnBuf; 
	}       

	string inline ContentProcessor::dnsmasqProcessor::getRawDNS(string originLine) {
		string returnBuf = "";
		for (int counter = originLine.find_last_of("/") + 1; counter < (int)originLine.size(); counter++) returnBuf += originLine[counter];
		return returnBuf;
	}

	string ContentProcessor::gfwlistProcessor::getRawDomain(string originLine) {

		string returnBuf = "";

		bool filterBuffer0 = originLine.find("[") != string::npos; // [Auto xxxx...
		bool filterBuffer1 = originLine.find("!") != string::npos; // Comments
		bool filterBuffer2 = originLine.find("@") != string::npos; // Non-proxy Lines
		bool filterBuffer3 = originLine.find("|") != string::npos; // Proxy Lines
		bool filterBuffer4 = originLine.find(".") != string::npos; // Link-Contained Lines
		bool filterBuffer5 = originLine.find("*") != string::npos;

		if (filterBuffer0 || filterBuffer1 || filterBuffer2 || filterBuffer5) return ""; // Skip unrelated lines

		else if (filterBuffer4) {
			if (filterBuffer3) {
				for (int i = (originLine.find_last_of("|") + 1); i < (int)originLine.size(); ++i) returnBuf += originLine[i];
				returnBuf += "\n";
				return returnBuf;
			}
		}
	}