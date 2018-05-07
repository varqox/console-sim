#include "http.h"

#include <iostream>
#include <string.h>

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
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
}

void HTTP::setBody(std::string s)
{
	body = s;
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
}

bool HTTP::send()
{
	CURLcode res = curl_easy_perform(curl);
	if(res != CURLE_OK)
		return false;
	curl_easy_cleanup(curl);

	return true;
}

std::string HTTP::getResponse()
{
	return response;
}

bool HTTP::logIn(std::string username, std::string password)
{
	setBody("username=" + username + "&password=" + password + "&persistent-login=on&csfr_token=");

	if(!send())
		return false;

	return true;
}