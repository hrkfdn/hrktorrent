#include "hrktorrent.h"

CIPFilter* IPFilter = 0;

CIPFilter::CIPFilter()
{
	if(!Settings->GetI("ipfilter")) {
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

	libtorrent::ip_filter::filter_tuple_t filters = _ipfilter.export_filter();
	unsigned int entrycount = filters.get<0>().size() + filters.get<1>().size();
	std::cout << entrycount << " entries imported." << std::endl;

	config.close();
}

CIPFilter::~CIPFilter()
{
	Core->VerbosePrint("IPFilter", "Destroying IPFilter class.");
}


bool
CIPFilter::AddRule(std::string start, std::string end)
{
	if(start.empty() || end.empty() || !_active) return false;

	libtorrent::address astart, aend;
	try {
		astart = libtorrent::address::from_string(start);
		aend = libtorrent::address::from_string(end);
	} catch(boost::system::system_error& e) {
		return false; // invalid rule
	}

	try {
		_ipfilter.add_rule(astart, aend, libtorrent::ip_filter::blocked);
	} catch(std::exception& e) {
		return false;
	}

	return true;
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

	if(!AddRule(start,end))
		Core->VerbosePrint("Settings", "Error adding line to ip filter.");
}
