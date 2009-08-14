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

		inline libtorrent::session* GetSession() { return _session; }
		inline libtorrent::torrent_handle* GetTorrent() { return &_torrent; }
		inline bool isRunning() { return _running == true; }
	private:
		bool _running;
		libtorrent::session* _session;
		libtorrent::torrent_handle _torrent; // TODO: multiple torrents?

		void LoadDHT();
		void SaveDHT();
		void ScheduleSignal(int signo);

		int _argc;
		char** _argv;

		bool _verbose;
};
extern CCore* Core;

#endif
