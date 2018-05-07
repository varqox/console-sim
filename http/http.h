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

private:
	std::string response;
	CURL *curl;
	static size_t write_to_str(void*, size_t, size_t, std::string*);
};
