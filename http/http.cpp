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
	uri = "https://oboz.sim.ugo.si" + path;
	curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_str);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.47.0");
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, ".cookies");
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ".cookies");
}

HTTP& HTTP::setBody(std::string content)
{
	body = content;
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
	return *this;
}

HTTP& HTTP::send()
{
	std::string s;
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	CURLcode res = curl_easy_perform(curl);

	if(res != CURLE_OK)
		std::cerr << "CONNECTION FAILED!\n";
	else
		response = s;

	curl_easy_cleanup(curl);
	return *this;
}

std::string HTTP::getResponse()
{
	return response;
}

HTTP& HTTP::logIn(std::string username, std::string password)
{
	remove(".cookies");
	setBody("username=" + username + "&password=" + password + "&persistent-login=on&csfr_token=");
	return send();
}

HTTP& HTTP::setDebug(long enable)
{
	curl_easy_setopt(curl, CURLOPT_VERBOSE, enable);
	return *this;
}
