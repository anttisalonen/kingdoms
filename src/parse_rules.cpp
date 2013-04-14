#include <sstream>
#include <fstream>
#include <exception>

#include "parse_rules.h"
#include "config.h"
#include "paths.h"

std::vector<std::vector<std::string> > parser(const std::string& filepath, 
		unsigned int num_fields, bool vararg = false)
{
	using namespace std;
	ifstream ifs(filepath.c_str(), ifstream::in);
	if(!ifs.good()) {
		std::string s("Could not open file ");
		s += filepath;
		throw std::runtime_error(std::string(s));
	}
	int linenum = 0;
	vector<vector<string> > results;
	while(ifs.good()) {
		linenum++;
		string line;
		getline(ifs, line);
		bool quoting = false;
		bool filling = false;
		bool escaping = false;
		vector<string> values;
		string value;
		for(unsigned int i = 0; i < line.length(); ++i) {
			if(escaping) {
				value += line[i];
				escaping = false;
			}
			else if(((!quoting && (line[i] == ' ' || line[i] == '\t')) || (quoting && line[i] == '"')) 
					&& filling) {
				values.push_back(value);
				value = "";
				quoting = false;
				filling = false;
				escaping = false;
			}
			else if(line[i] == '"') {
				filling = true;
				quoting = true;
			}
			else if(!quoting && line[i] == '#') {
				break;
			}
			else if(line[i] == '\\') {
				escaping = true;
			}
			else if(quoting || !(line[i] == ' ' || line[i] == '\t')) {
				filling = true;
				value += line[i];
			}
		}
		// add last parsed token to values
		if(value.size())
			values.push_back(value);

		if(values.size() == num_fields || (vararg && values.size() > num_fields)) {
			results.push_back(values);
		}
		else if(values.size() != 0) {
			fprintf(stderr, "Warning: parsing %s - line %d: parsed %lu "
					"tokens - wanted %d:\n",
					filepath.c_str(),
					linenum, values.size(), num_fields);
			for(unsigned int i = 0; i < values.size(); i++) {
				fprintf(stderr, "\t%s\n", values[i].c_str());
			}
		}
	}
	ifs.close();
	return results;
}

bool get_flag(const std::string& s, unsigned int i)
{
	if(s.length() <= i)
		return false;
	else
		return s[i] != '0';
}

typedef std::vector<std::vector<std::string> > parse_result;

std::vector<civilization*> parse_civs_config(const std::string& fp)
{
	std::vector<civilization*> civs;
	parse_result pcivs = parser(fp, 5, true);
	for(unsigned int i = 0; i < pcivs.size(); i++) {
		civs.push_back(new civilization(pcivs[i][0], i, 
					color(stoi(pcivs[i][1]),
						stoi(pcivs[i][2]),
						stoi(pcivs[i][3])),
					NULL, pcivs[i].begin() + 4, 
					pcivs[i].end(), NULL, NULL, false));
	}
	return civs;
}

unit_bonus get_unit_bonus(const std::string& s)
{
	unit_bonus b;
	b.type = unit_bonus_none;
	size_t ind = s.find_first_of(":");
	if(ind == std::string::npos) {
		return b;
	}
	std::string type(s, 0, ind);
	std::string value(s, ind + 1);
	if(type[0] == 'c') {
		b.type = unit_bonus_city;
	}
	else {
		int grps = stoi(type);
		if(grps == 0) {
			return b;
		}
		else {
			b.type = unit_bonus_group;
			b.bonus_data.group_mask = grps;
		}
	}
	b.bonus_amount = stoi(value);
	return b;
}

unit_configuration_map parse_unit_config(const std::string& fp)
{
	parse_result units = parser(fp, 9 + max_num_unit_needed_resources + max_num_unit_bonuses);
	unit_configuration_map uconfmap;
	for(unsigned int i = 0; i < units.size(); i++) {
		unit_configuration u;
		u.unit_name = units[i][0];
		u.max_moves = stoi(units[i][1]);
		u.max_strength = stoi(units[i][2]);
		u.production_cost = stoi(units[i][3]);
		u.population_cost = stoi(units[i][4]);
		u.needed_advance = stoi(units[i][5]);
		for(unsigned int j = 0; j < max_num_unit_needed_resources; j++) {
			u.needed_resources[j] = stoi(units[i][6 + j]);
		}
		u.carry_units = stoi(units[i][6 + max_num_unit_needed_resources]);
		u.unit_group_mask = stoi(units[i][7 + max_num_unit_needed_resources]);
		for(unsigned int j = 0; j < max_num_unit_bonuses; j++) {
			u.unit_bonuses[j] = get_unit_bonus(units[i][8 + max_num_unit_needed_resources + j]);
		}
		u.settler = get_flag(units[i][8 + max_num_unit_needed_resources + max_num_unit_bonuses], 0);
		u.worker = get_flag(units[i][8 + max_num_unit_needed_resources + max_num_unit_bonuses], 1);
		u.sea_unit = get_flag(units[i][8 + max_num_unit_needed_resources + max_num_unit_bonuses], 2);
		u.ocean_unit = get_flag(units[i][8 + max_num_unit_needed_resources + max_num_unit_bonuses], 3);
		uconfmap.insert(std::make_pair(i, u));
	}
	return uconfmap;
}

advance_map parse_advance_config(const std::string& fp)
{
	advance_map amap;
	parse_result discoveries = parser(fp, 6);
	for(unsigned int i = 0; i < discoveries.size(); i++) {
		advance a;
		a.advance_id = i + 1;
		a.advance_name = discoveries[i][0];
		a.cost = stoi(discoveries[i][1]);
		for(int j = 0; j < max_num_needed_advances; j++) {
			a.needed_advances[j] = stoi(discoveries[i][j + 2]);
		}
		amap.insert(std::make_pair(a.advance_id, a));
	}
	return amap;
}

city_improv_map parse_city_improv_config(const std::string& fp)
{
	parse_result improvs = parser(fp, 9);
	city_improv_map cimap;
	for(unsigned int i = 0; i < improvs.size(); i++) {
		city_improvement a;
		a.improv_id = i + 1;
		a.improv_name = improvs[i][0];
		a.cost = stoi(improvs[i][1]);
		a.needed_advance = stoi(improvs[i][2]);
		a.comm_bonus = stoi(improvs[i][3]);
		a.science_bonus = stoi(improvs[i][4]);
		a.defense_bonus = stoi(improvs[i][5]);
		a.culture = stoi(improvs[i][6]);
		a.happiness = stoi(improvs[i][7]);
		a.barracks = get_flag(improvs[i][8], 0);
		a.granary = get_flag(improvs[i][8], 1);
		a.palace = get_flag(improvs[i][8], 2);
		cimap.insert(std::make_pair(a.improv_id, a));
	}
	return cimap;
}

resource_configuration parse_terrain_config(const std::string& fp)
{
	resource_configuration resconf;
	resconf.city_food_bonus = 1;
	resconf.city_prod_bonus = 1;
	resconf.city_comm_bonus = 0; // compensated by road
	resconf.irrigation_needed_turns = 3;
	resconf.mine_needed_turns = 4;
	resconf.road_needed_turns = 2;
	parse_result terrains = parser(fp, 8);

	if(terrains.size() >= (int)num_terrain_types) {
		fprintf(stderr, "Warning: more than %d parsed terrains will be ignored in %s.\n",
				num_terrain_types, std::string(fp).c_str());
	}
	for(unsigned int i = 0; i < std::min<unsigned int>(num_terrain_types, terrains.size()); i++) {
		resconf.resource_name[i] = terrains[i][0];
		resconf.terrain_food_values[i] = stoi(terrains[i][1]);
		resconf.terrain_prod_values[i] = stoi(terrains[i][2]);
		resconf.terrain_comm_values[i] = stoi(terrains[i][3]);
		resconf.terrain_type[i] = stoi(terrains[i][4]);
		resconf.temperature[i] = stoi(terrains[i][5]);
		resconf.humidity[i] = stoi(terrains[i][6]);
		resconf.found_city[i] = get_flag(terrains[i][7], 0);
		resconf.irrigatable[i] = get_flag(terrains[i][7], 1);
		resconf.mineable[i] = get_flag(terrains[i][7], 2);
		resconf.roadable[i] = get_flag(terrains[i][7], 3);
	}
	return resconf;
}

government_map parse_government_config(const std::string& fp)
{
	government_map govmap;
	parse_result governments = parser(fp, 6);

	for(unsigned int i = 0; i < governments.size(); i++) {
		government gov(i + 1, governments[i][0]);
		gov.needed_advance = stoi(governments[i][1]);
		gov.production_cap = stoi(governments[i][2]);
		gov.free_units = stoi(governments[i][3]);
		gov.city_units = stoi(governments[i][4]);
		gov.unit_cost = stoi(governments[i][5]);
		govmap.insert(std::make_pair(gov.gov_id, gov));
	}
	return govmap;
}

resource_map parse_resource_config(const std::string& fp)
{
	resource_map rmap;
	parse_result resources = parser(fp, 6 + max_num_resource_terrains * 2);

	for(unsigned int i = 0; i < resources.size(); i++) {
		resource r;
		r.name = resources[i][0];
		r.food_bonus = stoi(resources[i][1]);
		r.prod_bonus = stoi(resources[i][2]);
		r.comm_bonus = stoi(resources[i][3]);
		r.luxury_bonus = stoi(resources[i][4]);
		r.needed_advance = stoi(resources[i][5]);
		for(unsigned int j = 0; j < max_num_resource_terrains; j++) {
			r.terrain[j] = stoi(resources[i][6 + j * 2]);
			r.terrain_abundance[j] = stoi(resources[i][7 + j * 2]);
		}
		rmap.insert(std::make_pair(i + 1, r));
	}
	return rmap;
}

void get_configuration(const std::string& ruleset_name,
		std::vector<civilization*>* civs,
		unit_configuration_map* units,
		advance_map* advances,
		city_improv_map* improvs,
		resource_configuration* terrains,
		government_map* governments,
		resource_map* resources)
{
	std::string prefix = get_rules_path(ruleset_name);
	if(civs)
		*civs = parse_civs_config(prefix + "/civs.txt");
	if(units)
		*units = parse_unit_config(prefix + "/units.txt");
	if(advances)
		*advances = parse_advance_config(prefix + "/discoveries.txt");
	if(improvs)
		*improvs = parse_city_improv_config(prefix + "/improvs.txt");
	if(terrains)
		*terrains = parse_terrain_config(prefix + "/terrain.txt");
	if(governments)
		*governments = parse_government_config(prefix + "/governments.txt");
	if(resources)
		*resources = parse_resource_config(prefix + "/resources.txt");
}


