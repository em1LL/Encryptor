// Application.h

#ifndef APPLICATION_H
#define APPLICATION_H

#include <string>

class Application
{
	// Private structure w/ configurable parameters which describes settings of our application
	struct Config
	{
		std::string filename; // Name of file to be encrypted
		std::string temp_filename; // Name of temp. file which can be used to save our original file if some error occurred
		std::string keyword; // Keyword to encrypt file
		std::string output; // Destination
		std::string type; // Encryption type
		std::string config_file; // Name of config file
	};

public:
	Application(int argc, char *argv[]); // Ctor which can be used to call private function parseConfig
	int exec(); // Main function of our application which can be used to determine whether it was successfully completed or not

private:
	Config m_config; // An instance of our settings structure

	int parseConfig(int argc, char *argv[]); // Receiving params from cmd line and config file
	void getData(); // Function which can be used to receive data to be encrypted
	void convertData(char data[], std::size_t size); // Main function which can be used to encrypt / decrypt file
	void outputData(char data[], std::size_t size); // Function which can be used to output encrypted / decrypted data

	// Required by rc4 encoding type
	unsigned char rc4i;
	unsigned char rc4j;
	char rc4arr[256];
	void rc4init(const std::string& code);
	void rc4(char* str, std::size_t len);

	// Specific types of encoding
#ifdef _WIN32
	void winEncrypt();
#endif
};

#endif // APPLICATION_H