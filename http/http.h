#pragma once

#include "../dependencies/sockets.hpp"

class HTTP
{
public:
	enum Method {};
	HTTP(Method, std::string);

	enum Header {};
	void addHeader(Header, std::string);

	void setBody(std::string);

	bool send();
	std::string getResponse();

private:
};
