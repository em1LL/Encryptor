#ifndef MAKESTRING_H
#define MAKESTRING_H

#include <sstream>

class MakeString
{
public:
	template<class T>
	MakeString& operator<< (const T& arg)
	{
		m_stream << arg;
		return *this;
	}
	operator std::string() const
	{
		return m_stream.str();
	}

protected:
	std::stringstream m_stream;
};

#endif // MAKESTRING_H