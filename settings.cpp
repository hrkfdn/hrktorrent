#include "hrktorrent.h"

CSettings* Settings = 0;

CSettings::CSettings()
{
	/* apply default settings */
	settings["verbose"] = 1;

	settings["minport"] = 6881;
	settings["maxport"] = 6999;

	settings["maxup"] = 0;
	settings["maxdown"] = 0;

	settings["dht"] = 1;
	settings["seed"] = 1;
	settings["forcereannounce"] = 2;
	settings["ipfilter"] = 1;
}

CSettings::~CSettings()
{
	Core->VerbosePrint("Settings", "Destroying Settings class.");
}

void
CSettings::LoadConfig()
{
	_dir = getenv("HOME");
	_dir.append("/.hrktorrent/");
	
	std::string path = _dir;
	path.append("hrktorrent.rc");

	std::ifstream config(path.c_str(), std::ifstream::in);
	if(!config.is_open()) {
		Core->VerbosePrint("Settings", "Could not load config file. Will use default values.");
		return;
	}

	while(config.good()) {
		std::string line;
		getline(config, line);
		ParseLine(line);
	}

	config.close();
}

void
CSettings::ParseLine(std::string line)
{
	std::string var;
	std::string value;
	std::stringstream linestream;

	std::string::size_type loc = line.find_first_of("#");
	if(loc < line.size())
		line.resize(loc);

	linestream.str(line);

	linestream >> var;
	linestream >> value;
	if(var.empty()) return;

	settings[var] = atoi(value.c_str());
}

