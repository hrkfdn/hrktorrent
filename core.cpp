#include "hrktorrent.h"

CCore* Core = 0;
pthread_t statusthread;

/* constructor. init torrent session! */
CCore::CCore(int argc, char** argv)
{
	_argc = argc;
	_argv = argv;
	_running = true;

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
GetETA(libtorrent::size_type done, libtorrent::size_type wanted, libtorrent::size_type downspeed)
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

	libtorrent::session* s = Core->GetSession();
	libtorrent::torrent_handle* t = Core->GetTorrent();

	libtorrent::session_status sstatus;
	libtorrent::torrent_status tstatus;
	bool finished = false;
	int eta = 0, columns = 0, loopcount = 0;
	bool stdout_is_tty = false;

	stdout_is_tty = (isatty(STDOUT_FILENO) == 1);
	if (stdout_is_tty) {
		if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) {
			char errbuf[128];

			strerror_r(errno, errbuf, 128);
			std::cerr << "ioctl error: " << errbuf << std::endl;

			exit(EXIT_FAILURE);
		}
		columns = ws.ws_col;
	}


	while(Core->isRunning()) {
		if(Settings->GetI("forcereannounce") > 0 && loopcount >= Settings->GetI("forcereannounce")*60) {
			loopcount = 0;
			t->force_reannounce();
			Core->VerbosePrint("Core", "Reannouncing!\n");
		}

		sstatus = s->status();
		tstatus = t->status();
		eta = GetETA(tstatus.total_done, tstatus.total_wanted, (libtorrent::size_type)sstatus.download_rate);

		if(!finished && tstatus.total_wanted != 0 && tstatus.total_done == tstatus.total_wanted) {
			std::cout << "\nTorrent finished!" << std::endl;
			if(Settings->GetI("seed") == 0) {
				statusthread = 0;
				exit(EXIT_SUCCESS);
			}
			finished = true;
		}

		output << "ps: " << sstatus.num_peers;
		if(Settings->GetI("dht") > 0) 
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
CCore::LoadDHT()
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
CCore::SaveDHT()
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

static void
SignalHandler(int signo)
{
	Core->VerbosePrint("Core", "Received signal.");
	Core->Shutdown();
}

void
CCore::ScheduleSignal(int signo)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SignalHandler;
	sa.sa_flags = SA_RESETHAND;

	if (sigaction(signo, &sa, NULL) < 0) {
		char errbuf[128];

		strerror_r(errno, errbuf, 128);

		std::cerr << "sigaction error: " << errbuf << std::endl;
		exit(EXIT_FAILURE);
	}
}

void
CCore::Shutdown()
{
	_running = false;
	if(Settings->GetI("dht") > 0) {
		SaveDHT();
		_session->stop_dht();
	}

	if(statusthread) pthread_cancel(statusthread);
	std::cout << "\nShutting down hrktorrent. Please wait." << std::endl;

	exit(EXIT_SUCCESS);
}

int
CCore::Run()
{
	_verbose = (Settings->GetI("verbose") > 0);
	VerbosePrint("Core", "Verbose enabled.");
	VerbosePrint("Core", "Starting torrent session!");

	IPFilter = new CIPFilter();

	_session = new libtorrent::session(libtorrent::fingerprint("HT", MAJOR, MINOR, REVISION, TAG));

	if(Settings->GetI("upnp") > 0)
		_session->start_upnp();

	_session->listen_on(std::make_pair(Settings->GetI("minport"), Settings->GetI("maxport")));

	if(Settings->GetI("maxup") > 0)
		_session->set_upload_rate_limit(Settings->GetI("maxup")*1024);
	if(Settings->GetI("maxdown") > 0)
		_session->set_download_rate_limit(Settings->GetI("maxdown")*1024);

	try {
		libtorrent::add_torrent_params parms;

		boost::intrusive_ptr<libtorrent::torrent_info> info = new libtorrent::torrent_info(_argv[_argc-1]);
		boost::filesystem::path p(Settings->GetS("downloaddir"));
		boost::filesystem::create_directory(p);
		if(!boost::filesystem::exists(p)) {
			std::cerr << "Download directory does not exist/could not be created." << std::endl;
			return EXIT_FAILURE;
		}

		parms.save_path = p;
		parms.ti = info;
		parms.paused = false;

		_torrent = _session->add_torrent(parms);
	} catch(std::exception& e) {
		std::cerr << "Could not add torrent (" << e.what() <<")" << std::endl;
		return EXIT_FAILURE;
	}
	
	if(Settings->GetI("dht") > 0) {
		VerbosePrint("Core", "Starting DHT.");
		libtorrent::dht_settings dset;
		dset.service_port = _session->listen_port();
		_session->set_dht_settings(dset);
		LoadDHT();

		// known dht routers for bootstrapping
		_session->add_dht_router(std::make_pair(std::string("router.bittorrent.com"), 6881));
		_session->add_dht_router(std::make_pair(std::string("router.utorrent.com"), 6881));
		_session->add_dht_router(std::make_pair(std::string("router.bitcomet.com"), 6881));
	}

	if(Settings->GetI("ipfilter") > 0 && IPFilter->IsActive()) {
		_session->set_ip_filter(IPFilter->getFilter());
	}

	ScheduleSignal(SIGINT);
	std::cout << "\"CTRL-C\" shuts hrktorrent down.\n" <<  std::endl;

	pthread_create(&statusthread, NULL, StatusLoop, NULL);

	/*
	 * reading stdin does not work with output redirection or running the
	 * program in background
	 */
	/*
	char input;
	std::cin.unsetf(std::ios_base::skipws);
	std::cin >> input;
	*/

	/* wait for signal */
	pause();

	Shutdown();
}

void
CCore::VerbosePrint(std::string source, std::string message)
{
	if(!_verbose) return;
	std::cout << "[" << source << "] ";
	std::cout << message << std::endl;
}

