#pragma once

#include <curl/curl.h>
#include <string>

class HTTP
{
public:
	HTTP(std::string);

	HTTP&  setBody(std::string);

	HTTP&  send();
	std::string getResponse();

	HTTP&  logIn(std::string, std::string);

	HTTP&  setDebug(long);
private:
	std::string response;
	std::string uri, body;
	CURL *curl;
	static size_t write_to_str(void*, size_t, size_t, std::string*);
};
