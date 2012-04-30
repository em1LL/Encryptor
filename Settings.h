#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include "Singleton.h"

class Settings : public Singleton<Settings>
{
private:
	Settings() : Singleton<Settings>() {}
	std::string filename_arg; // Name of file to be encrypted
	std::string code_arg; // Code word to encrypt file
	std::string new_filename_arg; // New filename

protected:
	friend class Singleton<Settings>;

public:
	void setFilename(const std::string& str) { filename_arg = str; }
	void setCode(const std::string& str) { code_arg = str; }
	void setNewFilename(const std::string& str) { new_filename_arg = str; }
	std::string getFilename() const { return filename_arg; }
	std::string getCode() const { return code_arg; }
	std::string getNewFilename() const { return new_filename_arg; }
};

#endif // SETTINGS_H