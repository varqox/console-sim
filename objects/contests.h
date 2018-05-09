#pragma once

#include "../dependencies/json.hpp"
#include "../http/http.h"
#include <string>
#include <vector>

using json = nlohmann::json;

class Contests
{
public:
	Contests();
	void dump();
private:
	class Contest
	{
	public:
		Contest(json);

		int id;
		std::string name;
		bool is_public;
		std::string user_mode;
		std::string actions;
	};

	json parsed;
	std::vector<Contest> list;
};