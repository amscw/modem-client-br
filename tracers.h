#ifndef TRACERS_H
#define TRACERS_H

#include <iostream>
#include <sstream>

#define TRACE(stream) {\
	std::cerr << "TRACE:" << __FILE__ << "(" << __FUNCTION__ << "): " << stream.str() << std::endl;\
	stream.str("");\
	stream.clear();\
}

#endif // TRACERS_H
