#include "http.h"

#include <iostream>

size_t HTTP::write_to_str(void *contents, size_t size, size_t nmemb, std::string *s)
{
	size_t new_len = size * nmemb, old_len = s->size();
	s->resize(new_len + old_len);
	std::copy((char*) contents, (char*) contents + new_len, s->begin() + old_len);
	return new_len;
}

HTTP::HTTP(std::string path)
{
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, ("https://oboz.sim.ugo.si" + path).c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_str);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
}

void HTTP::setBody(std::string s)
{
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s.c_str());
}

bool HTTP::send()
{
	CURLcode res = curl_easy_perform(curl);
	if(res != CURLE_OK)
		return false;
	curl_easy_cleanup(curl);

	std::cout << response << std::endl;
	return true;
}

std::string HTTP::getResponse()
{
	return response;
}
