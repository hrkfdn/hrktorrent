#ifndef _SETTINGS_H
#define _SETTINGS_H

class CSettings
{
	public:
		CSettings();
		void LoadConfig();
		void ParseLine(std::string line);

		inline void Set(std::string name, int value) { settings[name] = value; }
		inline int Get(std::string name) { return settings[name]; }
		inline std::string* getDir() { return &_dir; }

		inline std::map<std::string, int>* getSettingMap() { return &settings; }
	private:
		std::map<std::string, int> settings;
		std::string _dir;
};
extern CSettings* Settings;

#endif
