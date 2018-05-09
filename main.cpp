#include "http/http.h"
#include "objects/contests.h"

#include <iostream>
#include <curl/curl.h>

int main()
{
	curl_global_init(CURL_GLOBAL_DEFAULT);

	HTTP("/login").logIn("console-sim", "xd");
	Contests().dump();

	curl_global_cleanup();
}
