#include "http/http.h"

#include <iostream>
#include <curl/curl.h>

int main()
{
	curl_global_init(CURL_GLOBAL_DEFAULT);

	HTTP http("/api/contests");
	http.send();

	curl_global_cleanup();
}
