#ifndef _SETTINGS_H
#define _SETTINGS_H

enum { TYPE_INT = 0, TYPE_STRING };

typedef struct
{
	int type;
	int ival;
	std::string sval;
} setting_t;

class CSettings
{
	public:
		CSettings();
		~CSettings();
		void LoadConfig();
		void ParseLine(std::string line);

		void Set(std::string name, int ival) { settings[name].type = TYPE_INT; settings[name].ival = ival; }
		void Set(std::string name, std::string sval) { settings[name].type = TYPE_STRING; settings[name].sval = sval; }

		int GetI(std::string name) { return settings[name].ival; }
		std::string GetS(std::string name) { return settings[name].sval; }

		inline std::string* getDir() { return &_dir; }

		inline std::map<std::string, setting_t>* getSettingMap() { return &settings; }
	private:
		std::map<std::string, setting_t> settings;
		std::string _dir;
};
extern CSettings* Settings;

#endif
