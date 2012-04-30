#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <iostream>
#include <string>
#include "Singleton.h"

class Logger : public Singleton<Logger>
{
private:
	Logger() : Singleton<Logger>() {}
	std::string log_destination; // Where we'll report useful info about errors

protected:
	friend class Singleton<Logger>;

public:
	void log(const std::string& logMessage) const;
	void setDestination(const std::string& str) { log_destination = str; }
	std::string getDestination() const { return log_destination; }
};

void Logger::log(const std::string& logMessage) const
{
	if (log_destination == "stderr")
	{
		std::cerr << logMessage;
	}
	else
	{
		try
		{
			std::ofstream f;
			f.exceptions(std::ios_base::failbit | std::ios_base::badbit);
			f.open(log_destination);
			f << logMessage;
		}
		catch (const std::ios_base::failure& e)
		{
			std::cerr << "*** An error occurred while write data to log file: " << e.what() << '\n';
		}
	}

	exit(EXIT_FAILURE);
}

#endif // LOGGER_H