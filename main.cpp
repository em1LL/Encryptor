#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
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

namespace fs = boost::filesystem;
namespace po = boost::program_options;

void renameFile(boost::filesystem::path& filename)
{
	// String full of valid characters for file naming
	const std::string temp_data = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.";

	std::string stem = filename.stem().string();
	const std::string extension = filename.extension().string();

	// Encrypt filename
	for (
		std::string::size_type i = 0
		, stem_size = stem.size()
		, temp_data_size = temp_data.size()
		; i < stem_size
		; ++i
		)
	{
		auto pos = temp_data.find(stem[i], 0);
		if (pos != std::string::npos)
		{
			/// \todo Tricky code better to have as a separate function w/ unit-tests
			/// \note But particularly this part better to replace w/ lookup table
			stem[i] = temp_data.at(temp_data_size - 1 - pos);
		}
		/// \bug Have u ever think about non English letters in filenames? :)
		/// What do u think would heppened?
		/// \todo Fix this nasty bug :)
	}

	filename = stem + extension;
}

int main(int argc, char *argv[])
{
	// Receiving params from command line using boost::program_options
	po::options_description description("Allowed options");
	description.add_options()
		("help,h", "Produce help message")
		("filename,f", po::value<std::string>(), "Set 'filename'")
		("code,c", po::value<std::string>(), "Set 'code'")
		("rename,r", "Encrypt name of file")
		("interactive,i", "Interactive mode");

	po::variables_map options; // Our params will be stored here
	try
	{
		po::store(po::parse_command_line(argc, argv, description), options); // Trying to parse command line arguments
	}
	catch (const std::exception& e)
	{
		std::cerr << "*** Error: An error occurred while parsing command line params: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	// If there are no params or user tried to showing help message
	if (options.empty() || options.count("help"))
	{
		std::cout << description << std::endl;
		return EXIT_FAILURE;
	}

	const std::string filename_arg = options["filename"].as<std::string>(); // Name of file to be encrypted
	const std::string code_arg = options["code"].as<std::string>(); // Code word to encrypt file
	/// \todo Add a real params validation code here
	assert(
		"After this point all input params expected to be valid" &&
		filename_arg.size() &&  // expect non empty filename
		code_arg.size() // expect non empty key string
		);

	fs::path filename(filename_arg);
	/// \todo Reduce scope of the input stream instance
	/// The rule is: release any resource (delete instance) when it doesn't needed anymore...
	fs::ifstream f_in;
	f_in.exceptions(std::ios_base::failbit | std::ios_base::badbit);

	/// \note Better to use a container w/ contiguous memory guarantee,
	/// to avoid reallocations
	std::vector<char> data;                                 // Input data container
	size_t size = 0;                                        // Set to zero for future check...
	try
	{
		/// \todo Check if file size is bigger than some threshold
		/// it would be better to do the job using blocks...
		/// \attention Never use seek+tell+seek back to detect file size!
		/// The correct way to get file size is to use \e stat(2)!
		size = fs::file_size(filename);      // Get file size
		/// \todo The rest has meaning only for files w/ some content,
		/// otherwise (file size == 0) there is nothing to do...
		assert("Implementation required for empty files!" && size);
		data.resize(size);                                  // Preallocate a buffer for file's data
		f_in.open(filename, std::ios::binary);                 // Keep std::ios::binary flag to support some special sequences like new-line characters
		f_in.read(&data[0], size);                             // Get whole file into memory
		f_in.close();
	}
	/// \note Catch as \c sed::exception, cuz \c boost::filesystem::file_size may throw as well
	catch (const std::exception& e)
	{
		std::cerr << "*** Error: An error occurred while opening / reading data from file: "
			<< e.what() << std::endl;
		return EXIT_FAILURE;
	}
	assert("Sanity (paranoid) check" && size && !data.empty());

	/// \todo Better to unroll this loop a little manually to gain some performance!
	for (
		std::string::size_type i = 0
		, j = 0
		, code_size = code_arg.size()           // NOTE avoid to calculate size on every iteration...
		; i < size
		; ++i
		/// \todo Candidate for <em>"extract method"</em> refactoring
		, j = (++j == code_size) ? 0 : j
		)
	{
		data[i] ^= code_arg[j];                                 // THE MAIN (DAMN TRICKY ALGORITHM :)
	}

	if (options.count("rename"))
	{
		renameFile(filename);
		std::cout << std::left << std::setw(18) << "New filename: " << filename << std::endl;
	}

	std::cout << std::left << std::setw(18) << "Bytes encrypted: " << size << std::endl;

	if (fs::exists(filename) && options.count("interactive"))
	{
		std::cout << "File associated with name " << filename << " already exists. Do you want to continue anyway? Y/N ";

		for (char c = std::cin.get(); c != 'Y'; c = std::cin.get())
		{
			std::cout << "As u wish commander... :)\n";
			return EXIT_FAILURE;
		}
	}

	/// \todo This would work but there is some 'problems' possible:
	/// file open could be really slooooow in some circumstances... so,
	/// if there wasn't rename operation requested, better to reuse already opened file...
	/// \todo And again: reduce scope of the output stream instance...
	fs::ofstream f_out;
	f_out.exceptions(std::ios_base::failbit | std::ios_base::badbit);

	try
	{
		assert("Stream expected to be valid at this point" && f_out.good());
		f_out.open(filename, std::ios::binary);
		f_out.write(data.data(), size);                         // Write whole data at once
		f_out.close();
	}
	catch (const std::exception& e)
	{
		std::cerr << "*** Error: An error occurred while opening / write data to file: "
			<< e.what() << std::endl;
		return EXIT_FAILURE;
	}

	// If our old filename does not equal to new filename, trying to delete old unencrypted file
	if (filename != filename_arg && !fs::remove(filename_arg))
	{
		std::cerr << "An error occurred while deleting file" << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}