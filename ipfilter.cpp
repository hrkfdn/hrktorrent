#include "hrktorrent.h"

CIPFilter* IPFilter = 0;

CIPFilter::CIPFilter()
{
	if(!Settings->Get("ipfilter")) {
		_active = false;
		return;
	}
	else
		_active = true;

	std::string path = *Settings->getDir();
	path.append("ipfilter.dat");

	std::ifstream config(path.c_str(), std::ifstream::in);
	if(!config.is_open()) {
		Core->VerbosePrint("Settings", "Unable to load ip filter from file. Aborting.");
		return;
	}

	std::cout << "Loading IPFilter. This could probably take a while!" << std::endl;
	
	while(config.good()) {
		std::string line;
		getline(config, line);
		ParseFilterLine(line);
	}

	config.close();
}


bool
CIPFilter::AddRule(std::string start, std::string end)
{
	if(start.empty() || end.empty() || !_active) return false;

	libtorrent::address astart, aend;
	astart = libtorrent::address::from_string(start);
	aend = libtorrent::address::from_string(end);

	try {
		_ipfilter.add_rule(astart, aend, libtorrent::ip_filter::blocked);
	} catch(std::exception& e) {
		return false;
	}

	return true;
}

std::string
CIPFilter::MakeValidIP(std::string ip)
{
	std::stringstream out;
    for(int i = 0; i < 4; i++) {
		const char* substr = ip.substr(i*3 + i, 3).c_str();
		out << atoi(substr);
		if(i < 3)
			out << ".";
	}
    return out.str();
}

void
CIPFilter::ParseFilterLine(std::string line)
{
	if(line.size() == 0 || line.size() < 5 ) return;

	std::string start;
	std::string end;

	std::string::size_type loc = line.find_first_of("-");
	std::string::size_type loc2 = line.find_first_of(",");

	start.assign(line);
	start.resize(loc-1);

	end.assign(line);
	end.erase(0,loc+2);
	end.resize(loc-1);

	std::string new_start = MakeValidIP(start);
	std::string new_end = MakeValidIP(end);

	if(!AddRule(new_start,new_end))
		Core->VerbosePrint("Settings", "Error adding line to ip filter.");
}
