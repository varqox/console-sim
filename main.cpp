#include "http/http.h"

#include <iostream>
#include <curl/curl.h>

int main()
{
	curl_global_init(CURL_GLOBAL_DEFAULT);

	HTTP http("/login");
	http.logIn("console-sim", "xd");

	curl_global_cleanup();
}
