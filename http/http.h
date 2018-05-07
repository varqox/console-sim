#pragma once

#include <curl/curl.h>
#include <string>

class HTTP
{
public:
	HTTP(std::string);

	void setBody(std::string);

	bool send();
	std::string getResponse();

	bool logIn(std::string, std::string);
private:
	std::string response;
	std::string body;
	CURL *curl;
	static size_t write_to_str(void*, size_t, size_t, std::string*);
};
