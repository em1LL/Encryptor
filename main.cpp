#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include "MakeString.h"
#include "Logger.h"
#include "Settings.h"

namespace fs = boost::filesystem;
namespace ip = boost::interprocess;
namespace po = boost::program_options;

auto getLogger = Logger::getInstance();
auto getSettings = Settings::getInstance();

void parseCmdLine(int argc, char *argv[])
{
	// Receiving params from command line using boost::program_options
	po::options_description description("Allowed options");
	description.add_options()
		("help,h", "Produce help message")
		("filename,f", po::value<std::string>()->default_value(""), "Set 'filename'")
		("code,c", po::value<std::string>()->default_value(""), "Set 'code'")
		("rename,r", po::value<std::string>()->default_value(""), "Set new filename")
		("log,l", po::value<std::string>()->default_value("stderr"), "Set log destination");

	po::variables_map options; // Our params will be stored here
	try
	{
		po::store(po::parse_command_line(argc, argv, description), options); // Trying to parse command line arguments
	}
	// Missing arguments and syntax errors
	catch (const po::invalid_command_line_syntax &inv_syntax)
	{
		switch (inv_syntax.kind())
		{
		case po::invalid_syntax::missing_parameter:
			getLogger->log(MakeString() << "*** Missing argument for option '" << inv_syntax.tokens() << "'.\n");
			break;
		default:
			getLogger->log(MakeString() << "*** Syntax error, kind " << int(inv_syntax.kind()) << '\n');
		};
	}
	// Unknown options
	catch (const po::unknown_option &unkn_opt)
	{
		getLogger->log(MakeString() << "*** Unknown option '" << unkn_opt.get_option_name() << "'\n");
	}
	// Trying to catch any other exception
	catch (const std::exception& e)
	{
		getLogger->log(MakeString() << "*** Unknown error occurred while parsing command line params: " << e.what() << '\n');
	}

	// If there are no params or user tried to showing help message
	if (options.count("help"))
	{
		std::cout << description << std::endl;
		exit(EXIT_FAILURE);
	}

	getSettings->setFilename(options["filename"].as<std::string>());
	getSettings->setCode(options["code"].as<std::string>());
	getSettings->setNewFilename(options["rename"].as<std::string>());
	getLogger->setDestination(options["log"].as<std::string>());

	if (!getSettings->getFilename().size() ||
		!getSettings->getCode().size())
	{
		getLogger->log(MakeString() << "*** Implementation required for empty files");
	}
}

void encryptFile()
{
	fs::path filename(getSettings->getFilename());

	// Is file empty?
	if (fs::is_empty(filename))
	{
		getLogger->log(MakeString() << "*** Implementation required for empty files");
	}

	//Create a file mapping
	ip::file_mapping m_file(filename.string().c_str(), ip::read_write);

	//Map the whole file with read-write permissions in this process
	ip::mapped_region region(m_file, ip::read_write);

	//Get the address of the mapped region
	char * const data = static_cast <char*> (region.get_address());
	const std::size_t file_size = region.get_size();

	const std::string code_temp = getSettings->getCode();

	/// \todo Better to unroll this loop a little manually to gain some performance!
	for (
		std::string::size_type i = 0
		, j = 0
		, code_size = getSettings->getCode().size()           // NOTE avoid to calculate size on every iteration...
		; i < file_size
		; ++i
		/// \todo Candidate for <em>"extract method"</em> refactoring
		, j = (++j == code_size) ? 0 : j
		)
	{
		data[i] ^= code_temp[j];                                 // THE MAIN (DAMN TRICKY ALGORITHM :)
	}

	std::cout << "Bytes encrypted: " << file_size << std::endl;
}

void renameFile()
{
	if (getSettings->getNewFilename().size())
	{
		fs::rename(getSettings->getFilename(), getSettings->getNewFilename());
	}
}

int main(int argc, char *argv[])
{
	parseCmdLine(argc, argv);

	encryptFile();

	renameFile();

	return EXIT_SUCCESS;
}