#include "contest.h"

#include <iostream>

Contest::Contest(int _id)
{
	id = _id;
	parsed = json::parse(HTTP("/api/contest/c" + std::to_string(id)).send().getResponse());

	json contest_data = parsed[1];
	name = contest_data[1];
	is_public = contest_data[2];
	user_mode = contest_data[3];
	actions = contest_data[4];

	json rounds_data = parsed[2];
	for(json &d : rounds_data)
		rounds.emplace_back(d);

	json problems_data = parsed[3];
	for(json &d : problems_data)
	{
		Round::Problem p(d);
		for(Round &r : rounds)
		{
			if(r.id == p.round_id)
			{
				r.problems.emplace_back(d);
				break;
			}
		}
	}

	std::sort(rounds.begin(), rounds.end(), [&](Round &a, Round &b) {
		return a.item < b.item;
	});

	for(Round &r : rounds)
	{
		std::sort(r.problems.begin(), r.problems.end(), [&](Round::Problem &a, Round::Problem &b) {
			return a.item < b.item;
		});
	}
}

Contest::Round::Round(json data)
{
    id = data[0];
    name = data[1];
    item = data[2];
    ranking_exposure = data[3];
    begins = data[4];
    full_results = data[5];
    ends = data[6];
}

Contest::Round::Problem::Problem(json data)
{	
    id = data[0];    
    round_id = data[1];    
    problem_id = data[2];    
    problem_label = data[3];    
    name = data[4];    
    item = data[5];    
    final_selecting_method = data[6];    
    reveal_score = data[7];
}

void Contest::dump()
{
	std::cout << "id: " << id << "\n"
			  << "name: " << name << "\n"
			  << "is_public: " << is_public << "\n"
			  << "user_mode: " << user_mode << "\n"
			  << "actions " << actions << "\n";

	for(size_t i = 0; i < rounds.size(); i++)
	{
		std::cout << "round: " << i << "\n{\n"
				  << "    id: " << rounds[i].id << "\n"
				  << "    name: " << rounds[i].name << "\n"
				  << "    item: " << rounds[i].item << "\n"
				  << "    ranking_exposure: " << rounds[i].ranking_exposure << "\n"
				  << "    begins: " << rounds[i].begins << "\n"
				  << "    full_results: " << rounds[i].full_results << "\n"
				  << "    end: " << rounds[i].ends << "\n";
		for(size_t j = 0; j < rounds[i].problems.size(); j++)
		{
			std::cout << "    problem: " << j << "\n    {\n"
					  << "        id: " << rounds[i].problems[j].id << "\n"
					  << "        round_id: " << rounds[i].problems[j].round_id << "\n"
					  << "        problem_id: " << rounds[i].problems[j].problem_id << "\n"
					  << "        problem_label: " << rounds[i].problems[j].problem_label << "\n"
					  << "        name: " << rounds[i].problems[j].name << "\n"
					  << "        item: " << rounds[i].problems[j].item << "\n"
					  << "        final_selecting_method: " << rounds[i].problems[j].final_selecting_method << "\n"
					  << "        reveal_score: " << rounds[i].problems[j].reveal_score << "\n"
					  << "    }\n";
		}
		std::cout << "}\n";
	}
}