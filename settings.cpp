#include "hrktorrent.h"

CSettings* Settings = 0;

CSettings::CSettings()
{
	/* apply default settings */
	Set("verbose", 1);

	Set("minport", 6881);
	Set("maxport", 6999);

	Set("maxup", 0);
	Set("maxdown", 0);

	Set("dht", 1);
	Set("upnp", 1);
	Set("seed", 1);
	Set("forcereannounce", 2);
	Set("ipfilter", 1);

	Set("downloaddir", ".");
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
		/* workaround: Core->VerbosePrint uses non-initialized Core */
		std::cout << "[Settings] Could not load config file. Will use default values." << std::endl;

		/* cannot do: Core may not be properly initialized */
		/* Core->VerbosePrint("Settings", "Could not load config file. Will use default values.");*/
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

	switch(settings[var].type) {
		case TYPE_INT:
			Set(var, atoi(value.c_str()));
			break;
		case TYPE_STRING:
			Set(var, value);
			break;
	}
}

