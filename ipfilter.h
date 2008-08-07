#ifndef _IPFILTER_H
#define _IPFILTER_H

class CIPFilter
{
	public:
		CIPFilter();
		bool AddRule(std::string start, std::string end);
		libtorrent::ip_filter getFilter() { return _ipfilter; }
		bool IsActive() { return (_active == true); }

	private:
		void ParseFilterLine(std::string line);
		std::string MakeValidIP(std::string ip);
		libtorrent::ip_filter _ipfilter;

		bool _active;
};

extern CIPFilter* IPFilter;

#endif
