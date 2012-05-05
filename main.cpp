// main.cpp

// Project specific includes

#include "Application.h"

// Standard includes

// Boost
#include <boost/detail/lightweight_main.hpp>
// Standard Library
#include <iostream>

int cpp_main(int argc, char *argv[])
{
	// Make (initialized) an instance of application
	Application app(argc, argv);
	// Ready to run! (at this point all params are validated already, so we r ready to do the job)
	return app.exec();
}