#include "hrktorrent.h"

void
printusage()
{
	std::cout << APPNAME << " " << VERSION << " by hrkfdn (hrkfdn A T gmail D O T com) and other contributors\n";
	std::cout << "usage: " << APPNAME << " [options] torrent-file\n";
	std::cout << "NOTE: options provided by the commandline override configuration file entries!\n\n";

	std::cout << "\t--minport\t\t- start port range (default: 6881)\n";
	std::cout << "\t--maxport\t\t- end port range (default: 6999)\n";

	std::cout << "\t--maxdown\t\t- download speed limit (in kb/s) (default: unlimited)\n";
	std::cout << "\t--maxup\t\t\t- upload speed limit (in kb/s) (default: unlimited)\n";

	std::cout << "\t--nodht\t\t\t- disable dht (default: on)\n";
	std::cout << "\t--noupnp\t\t\t- disable upnp (default: on)\n";
	std::cout << "\t--noseed\t\t- disable seeding (default: on)\n";
	std::cout << "\t--forcereannounce\t- reannounce every X minutes (default: 2)\n";
	std::cout << "\t--downloaddir\t- set the target directory (default: .)\n";
	std::cout << "\t--ipfilter\t\t- enable ip filtering (default: off)\n\n";
	std::cout << "\t--verbose\t\t- print verbose messages (default: off)\n\n";

	std::cout << "example: hrktorrent --minport6500 --maxport6600 --nodht file.torrent\n";

	std::cout << std::endl;
}

void
parseargs(int argc, char** argv)
{
	for(unsigned int i = 1; i < argc; i++) {
		char* parg = argv[i];

		if(strstr(parg, "--noseed")) {
			Settings->Set("seed", 0);
			continue;
		}
		else if(strstr(parg, "--nodht")) {
			Settings->Set("dht", 0);
			continue;
		}
		else if(strstr(parg, "--noupnp")) {
			Settings->Set("upnp", 0);
			continue;
		}
		else if(strstr(parg, "--verbose")) {
			Settings->Set("verbose", 1);
			continue;
		}
		else if(strstr(parg, "--ipfilter")) {
			Settings->Set("ipfilter", 1);
			continue;
		}
	

		parg += 2;

		std::map<std::string, setting_t>::iterator it;
		for(it = Settings->getSettingMap()->begin(); it != Settings->getSettingMap()->end(); it++) {
			if(strstr(parg, it->first.c_str())) {
				char* val = parg + strlen(it->first.c_str());
				switch(it->second.type) {
					case TYPE_INT:
						it->second.ival = atoi(val);
						break;
					case TYPE_STRING:
						it->second.sval = val;
						break;
				}
			}
		}
	}
}

#define CLEAN(x) if(x) delete x
void
close()
{
	CLEAN(Settings);
	CLEAN(IPFilter);

	/* make sure this is always the last class to cleanup */
	CLEAN(Core);
}

int
main(int argc, char* argv[])
{
	atexit(close);

	using namespace std;
	int runstate = EXIT_FAILURE;

	if(argc < 2) {
		printusage();
		return EXIT_FAILURE;
	}

	Core = new CCore(argc, argv);
	parseargs(argc, argv);
	runstate = Core->Run();

	return runstate;
}
