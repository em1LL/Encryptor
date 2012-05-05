// Application.cpp

// Project specific includes

#include "Application.h"
#include "MakeString.h"

// Standard includes

// Boost
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
// Windows-specific
#ifdef _WIN32
#include <Windows.h>
#endif
// Standard Library
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = boost::filesystem;
namespace ip = boost::interprocess;
namespace po = boost::program_options;

Application::Application(int argc, char *argv[])
{
	m_config.temp_filename = "temp";

	if (parseConfig(argc, argv) == EXIT_FAILURE)
	{
		throw std::runtime_error("An error occurred while constructing Application object");
	}
}

int Application::exec()
{
	getData();

	return EXIT_SUCCESS;
}

int Application::parseConfig(int argc, char *argv[])
{
	// Receiving params from command line and config file using boost::program_options

	// Declare a group of options that will be 
	// allowed only on command line
	po::options_description generic("Generic options");
	generic.add_options()
		("help,h", "Produce help message")
		("config,c", po::value<std::string>(&m_config.config_file)->default_value("config.cfg"), "Set name of a file of a configuration");

	// Declare a group of options that will be 
	// allowed both on command line and in
	// config file
	po::options_description config("Configuration");
	config.add_options()
		("filename,f", po::value<std::string>(&m_config.filename)->default_value("stdin"), "Set filename")
		("keyword,k", po::value<std::string>(&m_config.keyword)->default_value("default"), "Set keyword")
		("output,o", po::value<std::string>(&m_config.output)->default_value("stdout"), "Set output destination")
		("type,t", po::value<std::string>(&m_config.type)->default_value("xor"), "Set encryption type");

	po::options_description cmdline_options;
	cmdline_options.add(generic).add(config);

	po::options_description config_file_options;
	config_file_options.add(config);

	po::variables_map options; // Our params will be stored here
	try
	{
		po::store(po::parse_command_line(argc, argv, cmdline_options), options); // Trying to parse command line arguments
		po::notify(options); // Store the values into regular variables

		std::ifstream ifs(m_config.config_file);
		if (!ifs)
		{
			std::cerr << "*** Can not open config file: " << m_config.config_file << '\n';
		}

		po::store(po::parse_config_file(ifs, config_file_options), options); // Trying to parse config file
		po::notify(options);
	}
	// Missing arguments and syntax errors
	catch (const po::invalid_command_line_syntax& inv_syntax)
	{
		switch (inv_syntax.kind())
		{
		case po::invalid_syntax::missing_parameter:
			std::cerr << "*** Missing argument for option '" << inv_syntax.tokens() << "'.\n";
			break;
		default:
			std::cerr << "*** Syntax error, kind " << int(inv_syntax.kind()) << '\n';
		};
		return EXIT_FAILURE;
	}
	// Unknown options
	catch (const po::unknown_option& unkn_opt)
	{
		std::cerr << "*** Unknown option '" << unkn_opt.get_option_name() << "'\n";
		return EXIT_FAILURE;
	}
	// Trying to catch any other exception
	catch (const std::exception& e)
	{
		std::cerr << "*** Unknown error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	// If there are no params or user tried to showing help message
	if (options.empty() || options.count("help"))
	{
		std::cout << cmdline_options << std::endl;
		return EXIT_SUCCESS;
	}

	if (!m_config.filename.size() ||
		!m_config.keyword.size() ||
		!m_config.output.size())
	{
		std::cerr << "*** Implementation required for empty files and keywords\n";
		return EXIT_FAILURE;
	}

	if (m_config.keyword == "default")
	{
		std::cerr << "*** Beware! Your keyword is 'default'\n";
	}

	// Show some user-friendly info about settings
	std::cout.setf(std::ios_base::left);
	std::cout << std::setw(19) << "- Config file: " << m_config.config_file << std::endl
		<< std::setw(19) << "- Encryption type: " << m_config.type << std::endl
		<< std::setw(19) << "- Filename: " << m_config.filename << std::endl
		<< std::setw(19) << "- Keyword: " << m_config.keyword << std::endl
		<< std::setw(19) << "- Output: " << m_config.output << std::endl;

	return EXIT_SUCCESS;
}

void Application::getData()
{
#ifdef _WIN32
	if (m_config.type == "win-e" ||
		m_config.type == "win-d")
	{
		winEncrypt();
		return; // We don't need to continue execution of this function because we don't want to encrypt file manually
	}
#endif

	if (m_config.filename == "stdin")
	{
		std::vector<char> data;
		std::copy(std::istream_iterator<char>(std::cin), std::istream_iterator<char>(), std::back_inserter(data));

		convertData(&data[0], data.size());
	}
	else
	{
		fs::path filename(m_config.filename);

		// Is file empty?
		if (fs::is_empty(filename))
		{
			throw std::runtime_error ("*** Implementation required for empty files\n");
		}

		fs::path temp(m_config.temp_filename);

		try
		{
			fs::copy_file(filename, temp, fs::copy_option::overwrite_if_exists);
		}
		catch(const fs::filesystem_error& e)
		{
			throw std::runtime_error (MakeString() << "*** An error occurred while copying file: " << e.what());
		}

		//Create a file mapping
		ip::file_mapping m_file(temp.string().c_str(), ip::read_write);

		//Map the whole file with read-write permissions in this process
		ip::mapped_region region(m_file, ip::read_write);

		//Get the address of the mapped region
		char * const data = static_cast <char*> (region.get_address());
		const std::size_t file_size = region.get_size();

		convertData(data, file_size);
	}
}

void Application::rc4init(const std::string& code)
{
	char tmp[256];
	rc4j = code.size();
	rc4i = 0;

	do
	{
		rc4arr[rc4i] = rc4i;
		tmp[rc4i] = code[rc4i % rc4j];
	} while (++rc4i);

	rc4i = rc4j = 0;
	char t;

	do
	{
		rc4j += rc4arr[rc4i] + tmp[rc4i];
		t = rc4arr[rc4i];
		rc4arr[rc4i] = rc4arr[rc4j];
		rc4arr[rc4j] = t;
	} while (++rc4i);
}

void Application::rc4(char* str, std::size_t len)
{
	char t;
	while (len--)
	{
		rc4j += rc4arr[++rc4i];
		t = rc4arr[rc4i];
		rc4arr[rc4i] = rc4arr[rc4j];
		rc4arr[rc4j] = t;
		*(str++) ^= rc4arr[(char)(rc4arr[rc4i] + rc4arr[rc4j])];
	}
}

void Application::convertData(char data[], std::size_t size)
{
	const std::string& code_temp = m_config.keyword;
	const std::string& type = m_config.type;

	if (type == "xor")
	{
		/// \todo Better to unroll this loop a little manually to gain some performance!
		for (
			std::string::size_type i = 0
			, j = 0
			, code_size = m_config.keyword.size()           // NOTE avoid to calculate size on every iteration...
			; i < size
			; ++i
			/// \todo Candidate for <em>"extract method"</em> refactoring
			, j = (++j == code_size) ? 0 : j
			)
		{
			data[i] ^= code_temp[j];                                 // THE MAIN (DAMN TRICKY ALGORITHM :)
		}
	}
	else if (type == "reverse")
	{
		std::reverse(data, data + size);
	}
	else if (type == "rc4")
	{
		rc4init(m_config.keyword);
		rc4(data, size);
	}
	else if (type == "cezar-e")
	{
		int temp;
		try
		{
			temp = boost::lexical_cast<int>(m_config.keyword);
		}
		catch (const boost::bad_lexical_cast& e)
		{
			throw std::runtime_error (MakeString() << "*** An error occurred while trying to cast code: " << e.what() << '\n');
		}

		if (temp <= 0 || temp > 127)
		{
			throw std::runtime_error ("Please try to use other code\n");
		}

		for (
			std::string::size_type i = 0
			; i < size
			; ++i
			)
		{
			char next = data[i] + temp;
			if (next > 127)
			{
				next -= 127;
			}
			data[i] = next;
		}
	}
	else if (type == "cezar-d")
	{
		int temp;
		try
		{
			temp = boost::lexical_cast<int>(m_config.keyword);
		}
		catch (const boost::bad_lexical_cast& e)
		{
			throw std::runtime_error (MakeString() << "*** An error occurred while trying to cast code: " << e.what() << '\n');
		}

		if (temp <= 0 || temp > 127)
		{
			throw std::runtime_error ("Please try to use other code\n");
		}

		for (
			std::string::size_type i = 0
			; i < size
			; ++i
			)
		{
			char next = data[i] - temp;
			if (next < 0)
			{
				next = 127 + next;
			}
			data[i] = next;
		}
	}
	else if ("dict")
	{
		const std::string& temp_data = m_config.keyword;

		// Encrypt filename
		for (
			std::string::size_type i = 0
			, temp_data_size = temp_data.size()
			; i < size
			; ++i
			)
		{
			auto pos = temp_data.find(data[i], 0);
			if (pos != std::string::npos)
			{
				/// \todo Tricky code better to have as a separate function w/ unit-tests
				/// \note But particularly this part better to replace w/ lookup table
				data[i] = temp_data.at(temp_data_size - 1 - pos);
			}
			/// \bug Have u ever think about non English letters in filenames? :)
			/// What do u think would heppened?
			/// \todo Fix this nasty bug :)
		}
	}
	else
	{
		throw std::runtime_error ("*** Unknown encoding type\n");
	}

	std::cout << "Bytes encrypted: " << size << std::endl;

	outputData(data, size);
}

#ifdef _WIN32
void Application::winEncrypt()
{
	if (m_config.filename == "stdin")
	{
		throw std::runtime_error ("*** You need to use other input device for win-specific encryption\n");
	}

	if (m_config.type == "win-e")
	{
		if (!EncryptFile(m_config.filename.c_str()))
		{
			throw std::runtime_error (MakeString() << "*** An error occurred while encrypting file. GetLastError: " << GetLastError() << '\n');
		}
	}
	else if (m_config.type == "win-d")
	{
		if (!DecryptFile(m_config.filename.c_str(), 0))
		{
			throw std::runtime_error (MakeString() << "*** An error occurred while encrypting file. GetLastError: " << GetLastError() << '\n');
		}
	}

	if (m_config.output == "stdout")
	{
		throw std::runtime_error ("*** You need to use other output device for win-specific encryption\n");
	}

	if (!CopyFile(m_config.filename.c_str(), m_config.output.c_str(), FALSE))
	{
		throw std::runtime_error (MakeString() << "*** An error occurred while copying file. GetLastError: " << GetLastError() << '\n');
	}
}
#endif

void Application::outputData(char data[], std::size_t size)
{
	if (m_config.output == "stdout")
	{
		std::copy(data, data + size, std::ostream_iterator<char>(std::cout));
	}
	else
	{
		try
		{
			fs::copy_file(m_config.temp_filename, m_config.filename, fs::copy_option::overwrite_if_exists);

			if (fs::exists(m_config.temp_filename))
			{
				ip::file_mapping::remove(m_config.temp_filename.c_str());
			}
		}
		catch (const fs::filesystem_error& e)
		{
			throw std::runtime_error (MakeString() << "*** An error occurred while copying and removing file: " << e.what());
		}
	}
}