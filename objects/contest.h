#pragma once

#include "../dependencies/json.hpp"
#include "../http/http.h"
#include <string>
#include <vector>

using json = nlohmann::json;

class Contest
{
public:
	Contest(int);
	void dump();
private:
	json parsed;
	int id;
	std::string name;
	bool is_public;
	std::string user_mode;
	std::string actions;

	class Round
	{
	public:
		Round(json);
	    int id;
	    std::string name;
	    int item;
	    std::string ranking_exposure;
	    std::string begins;
	    std::string full_results;
	    std::string ends;

		class Problem
		{
		public:
			Problem(json);
			int id;
			int round_id;
			int problem_id;
			std::string problem_label;
			std::string name;
			int item;
			std::string final_selecting_method;
			bool reveal_score;
		};
		std::vector<Problem> problems;
	};

	std::vector<Round> rounds;
};