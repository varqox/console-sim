#include "http/http.h"

#include <iostream>
#include <curl/curl.h>

int main()
{
	curl_global_init(CURL_GLOBAL_DEFAULT);

	HTTP("/login").logIn("console-sim", "xd");

	std::cout << HTTP("/api/contests").send().getResponse() << std::endl;

	curl_global_cleanup();
}
