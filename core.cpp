#include "hrktorrent.h"

CCore* Core = 0;
pthread_t statusthread;

/* constructor. init torrent session! */
CCore::CCore(int argc, char** argv)
{
	_argc = argc;
	_argv = argv;

	Settings = new CSettings();
	std::cout << "Initializing " << APPNAME << " " << VERSION << std::endl;

	Settings->LoadConfig();
}

/* destructor. free/delete garbage! */
CCore::~CCore()
{
	VerbosePrint("Core", "Destructing core class.");
	delete _session;
}

unsigned int
geteta(libtorrent::size_type done, libtorrent::size_type wanted, libtorrent::size_type downspeed)
{
	libtorrent::size_type delta = wanted - done;
	if(downspeed > 0)
		return (delta / downspeed);
	else
		return 0;
}
	

void*
CCore::StatusLoop(void* data)
{
	int curx, cury;
	std::string s_output;
	std::stringstream output;
	
	struct winsize ws;

	libtorrent::session* s = Core->getSession();
	libtorrent::torrent_handle* t = Core->getTorrent();

	libtorrent::session_status sstatus;
	libtorrent::torrent_status tstatus;
	bool finished = false;
	int eta = 0, columns = 0, loopcount = 0;
	bool stdout_is_tty = false;

	stdout_is_tty = (isatty(STDOUT_FILENO) == 1);
	if (stdout_is_tty) {
		if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) {
			char errbuf[128];
			std::string errstring;

			strerror_r(errno, errbuf, 128);
			errstring = "ioctl: ";
			errstring.append((const char *) errbuf);

			Core->VerbosePrint("Core", errstring);
			exit(EXIT_FAILURE);
		}
		columns = ws.ws_col;
	}


	while(true) {
		if(Settings->Get("forcereannounce") > 0 && loopcount >= Settings->Get("forcereannounce")*60) {
			loopcount = 0;
			t->force_reannounce();
			Core->VerbosePrint("Core", "Reannouncing!\n");
		}

		sstatus = s->status();
		tstatus = t->status();
		eta = geteta(tstatus.total_done, tstatus.total_wanted, (libtorrent::size_type)sstatus.download_rate);

		if(!finished && tstatus.total_wanted != 0 && tstatus.total_done == tstatus.total_wanted) {
			std::cout << "\nTorrent finished!" << std::endl;
			if(Settings->Get("seed") == 0) {
				statusthread = 0;
				exit(EXIT_SUCCESS);
			}
			finished = true;
		}

		output << "ps: " << sstatus.num_peers;
		if(Settings->Get("dht") > 0) 
			output << ", dht: " << sstatus.dht_nodes;
		output << " <> ";
		output << "dl: " << Round(sstatus.download_rate/1024, 2) << "kb/s, ";
		output << "ul: " << Round(sstatus.upload_rate/1024, 2) << "kb/s <> ";
		output << "dld: " << tstatus.total_done/1048576 << "mb, ";
		output << "uld: " << sstatus.total_payload_upload/1048576 << "mb, ";
		output << "size: " << tstatus.total_wanted/1048576 << "mb <> ";
		output << "eta: ";
		if(eta > 216000) {
			output << eta/216000 << "d ";
			eta -= (eta/216000)*216000;
		}
		if(eta > 3600) {
			output << eta/3600 << "h ";
			eta -= (eta/3600)*3600;
		}
		if(eta > 60) {
			output << eta/60 << "m ";
			eta -= (eta/60)*60;
		}
		output << eta << "s";
		s_output = output.str();

		if(s_output.length() > columns) {
			s_output.resize(columns - 3);
			s_output.append("..");
		}
		else if(s_output.length() < columns) {
			for(int i = 0; i < s_output.length() - columns; i++)
				s_output.append(" ");
		}

		if (stdout_is_tty) {
			if(s_output.length() > columns) {
				s_output.resize(columns - 3);
				s_output.append("..");
			}
			else if(s_output.length() < columns) {
				for(int i = 0; i < s_output.length() - columns; i++)
					s_output.append(" ");
			}
		}
		if (stdout_is_tty) {
			std::cout << s_output.c_str() << "\r";
		}
		else {
			std::cout << s_output.c_str() << std::endl;
		}

	
		std::cout.flush();
		output.str("");

		sleep(1);
		loopcount++;
	}

	return 0;
}

void
CCore::loadDHT()
{
	std::string path = *Settings->getDir();
	path.append("dhtnodes");
	
	std::ifstream nodefile(path.c_str(), std::ios_base::binary);
	if(!nodefile.is_open()) {
		mkdir(Settings->getDir()->c_str(), 0755);
		VerbosePrint("DHT", "Could not load the nodefile.");
		_session->start_dht();
		return;
	}

	nodefile.unsetf(std::ios_base::skipws);

	try {
		libtorrent::entry state = libtorrent::bdecode(std::istream_iterator<char>(nodefile),
													  std::istream_iterator<char>());
		_session->start_dht(state);
		VerbosePrint("DHT", "DHT started with a nodefile.");
	}
	catch(std::exception& e) {
		std::cout << e.what() << std::endl;
		_session->start_dht();
	}
	nodefile.close();
}

void
CCore::saveDHT()
{
	std::string path = *Settings->getDir();
	path.append("dhtnodes");

	std::ofstream nodefile(path.c_str(), std::ios_base::binary);
	if(!nodefile.is_open()) {
		VerbosePrint("DHT", "Could not save the nodefile.");
		return;
	}

	nodefile.unsetf(std::ios_base::skipws);

	libtorrent::entry state = _session->dht_state();
	libtorrent::bencode(std::ostream_iterator<char>(nodefile), state);
	VerbosePrint("DHT", "Saved nodes.");
	nodefile.close();
}

int
CCore::Run()
{
	VerbosePrint("Core", "Verbose enabled.");
	VerbosePrint("Core", "Starting torrent session!");

	IPFilter = new CIPFilter();

	_session = new libtorrent::session(libtorrent::fingerprint("HT", MAJOR, MINOR, REVISION, TAG));
	_session->listen_on(std::make_pair(Settings->Get("minport"), Settings->Get("maxport")));

	if(Settings->Get("maxup") > 0)
		_session->set_upload_rate_limit(Settings->Get("maxup")*1024);
	if(Settings->Get("maxdown") > 0)
		_session->set_download_rate_limit(Settings->Get("maxdown")*1024);

	torrentfile.open(_argv[_argc-1], std::ios_base::binary);
	if(!torrentfile.is_open()) {
		std::cerr << "Could not open the torrent file!" << std::endl;
		return EXIT_FAILURE;
	}
	torrentfile.unsetf(std::ios_base::skipws);
	libtorrent::entry e = libtorrent::bdecode(std::istream_iterator<char>(torrentfile),
											  std::istream_iterator<char>());

	try {
		libtorrent::torrent_info *info = new libtorrent::torrent_info(e);
		_torrent = _session->add_torrent(info, "");
	}
	catch(std::exception& e) {
		std::cerr << "Corrupted torrent file." << std::endl;
		return EXIT_FAILURE;
	}
	
	if(Settings->Get("dht") > 0) {
		VerbosePrint("Core", "Starting DHT.");
		libtorrent::dht_settings dset;
		dset.service_port = _session->listen_port();
		_session->set_dht_settings(dset);
		loadDHT();

		// known dht routers for bootstrapping
		_session->add_dht_router(std::make_pair(std::string("router.bittorrent.com"), 6881));
		_session->add_dht_router(std::make_pair(std::string("router.utorrent.com"), 6881));
		_session->add_dht_router(std::make_pair(std::string("router.bitcomet.com"), 6881));
	}

	if(Settings->Get("ipfilter") > 0 && IPFilter->IsActive()) {
		_session->set_ip_filter(IPFilter->getFilter());
	}

	std::cout << "\"Return\" shuts hrktorrent down.\n" <<  std::endl;
	pthread_create(&statusthread, NULL, StatusLoop, NULL);

	char input;
	std::cin.unsetf(std::ios_base::skipws);
	std::cin >> input;

	if(Settings->Get("dht") > 0) {
		saveDHT();
		_session->stop_dht();
	}

	if(statusthread) pthread_cancel(statusthread);
	std::cout << "\nShutting down hrktorrent. Please wait." << std::endl;

	return EXIT_SUCCESS;
}

void
CCore::VerbosePrint(std::string source, std::string message)
{
	if(!Settings->Get("verbose")) return;
	std::cout << "[" << source << "] ";
	std::cout << message << std::endl;
}

