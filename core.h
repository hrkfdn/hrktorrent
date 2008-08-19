#ifndef _CORE_H
#define _CORE_H

class CCore
{
	public:
		CCore(int argc, char** argv);
		~CCore();

		int Run();
		void Shutdown();
		void VerbosePrint(std::string source, std::string message);
		static void* StatusLoop(void* data);

		inline libtorrent::session* getSession() { return _session; }
		inline libtorrent::torrent_handle* getTorrent() { return &_torrent; }
	private:
		libtorrent::session* _session;
		libtorrent::torrent_handle _torrent; // TODO: multiple torrents?

		void loadDHT();
		void saveDHT();

		int _argc;
		char** _argv;

		std::ifstream torrentfile;
		bool _verbose;
};
extern CCore* Core;

#endif
