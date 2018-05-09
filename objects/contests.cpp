#include "contests.h"

#include <iostream>

Contests::Contests()
{
	parsed = json::parse(HTTP("/api/contests").send().getResponse());
	//std::cout << parsed.dump(4) << "\n";
	for(int i = 1; i < (int)parsed.size(); i++)
		list.emplace_back(parsed[i]);
}

Contests::Contest::Contest(json data)
{
	//std::cout << data.dump(4) << "\n";
	id = data[0];
	name = data[1];
	is_public = data[2];
	user_mode = data[3];
	actions = data[4];
}

void Contests::dump()
{
	for(int i = 0; i < (int)list.size(); i++)
	{
		std::cout << i << ":\n"
				  << "    id: " << list[i].id << "\n"
				  << "    name: " << list[i].name << "\n"
				  << "    is_public: " << list[i].is_public << "\n"
				  << "    user_mode: " << list[i].user_mode << "\n"
				  << "    actions " << list[i].actions << "\n";
	}
}