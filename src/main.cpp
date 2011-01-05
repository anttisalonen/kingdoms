#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <execinfo.h>

#include <list>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>
#include <fstream>
#include <sstream>

#include <boost/bind/bind.hpp>
#include <boost/algorithm/string.hpp>

#define BOOST_FILESYSTEM_VERSION	3
#include <boost/filesystem.hpp>

#include "color.h"
#include "utils.h"
#include "sdl-utils.h"
#include "buf2d.h"
#include "civ.h"
#include "gui.h"
#include "ai.h"
#include "serialize.h"

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

#define QUOTE_(x)	#x
#define QUOTE(x)	QUOTE_(x)
#ifndef PREFIX
#define PREFIX			.
#endif

#define KINGDOMS_SHAREDIR	QUOTE(PREFIX)"/share/kingdoms/"
#define KINGDOMS_GFXDIR		KINGDOMS_SHAREDIR"/gfx/"
#define KINGDOMS_RULESDIR	KINGDOMS_SHAREDIR"/rules/"

static bool signal_received = false;

static bool observer = false;
static bool use_gui = true;
static bool ai_debug = false;
static int skip_rounds = 0;
static bool load = false;

static SDL_Surface* screen = NULL;
static TTF_Font* font = NULL;

void segv_handler(int sig)
{
	void* array[16];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 16);

	// print out all the frames to stderr
	fprintf(stderr, "Signal %d:\n", sig);
	backtrace_symbols_fd(array, size, 2);
	exit(EXIT_FAILURE);
}

void signal_handler(int sig)
{
	signal_received = true;
}

std::vector<std::vector<std::string> > parser(const std::string& filepath, 
		unsigned int num_fields, bool vararg = false)
{
	using namespace std;
	ifstream ifs(filepath.c_str(), ifstream::in);
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
			fprintf(stderr, "Warning: parsing %s - line %d: parsed %d "
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

int stoi(const std::string& s)
{
	int res = 0;
	std::stringstream(s) >> res;
	return res;
}

bool get_flag(const std::string& s, unsigned int i)
{
	if(s.length() <= i)
		return false;
	else
		return s[i] != '0';
}

typedef std::vector<std::vector<std::string> > parse_result;

std::vector<std::string> get_file_list(const std::string& prefix, const std::string& filepath)
{
	using namespace std;
	ifstream ifs(filepath.c_str(), ifstream::in);
	int linenum = 0;
	std::vector<std::string> results;
	while(ifs.good()) {
		linenum++;
		string line;
		getline(ifs, line);
		boost::trim(line);
		if(line.length())
			results.push_back(line.insert(0, prefix));
	}
	ifs.close();
	return results;
}

std::vector<civilization*> parse_civs_config(const std::string& fp)
{
	std::vector<civilization*> civs;
	parse_result pcivs = parser(fp, 5, true);
	for(unsigned int i = 0; i < pcivs.size(); i++) {
		civs.push_back(new civilization(pcivs[i][0], i, 
					color(stoi(pcivs[i][1]),
						stoi(pcivs[i][2]),
						stoi(pcivs[i][3])),
					NULL, false, pcivs[i].begin() + 4, 
					pcivs[i].end(), NULL));
	}
	return civs;
}

unit_configuration_map parse_unit_config(const std::string& fp)
{
	parse_result units = parser(fp, 7);
	unit_configuration_map uconfmap;
	for(unsigned int i = 0; i < units.size(); i++) {
		unit_configuration u;
		u.unit_name = units[i][0];
		u.max_moves = stoi(units[i][1]);
		u.max_strength = stoi(units[i][2]);
		u.production_cost = stoi(units[i][3]);
		u.needed_advance = stoi(units[i][4]);
		u.carry_units = stoi(units[i][5]);
		u.settler = get_flag(units[i][6], 0);
		u.worker = get_flag(units[i][6], 1);
		u.sea_unit = get_flag(units[i][6], 2);
		u.ocean_unit = get_flag(units[i][6], 3);
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

resource_configuration parse_resource_config(const std::string& fp)
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
	parse_result governments = parser(fp, 5);

	for(unsigned int i = 0; i < governments.size(); i++) {
		government gov(i + 1, governments[i][0]);
		gov.needed_advance = stoi(governments[i][1]);
		gov.free_units = stoi(governments[i][3]);
		gov.unit_cost = stoi(governments[i][4]);
		govmap.insert(std::make_pair(gov.gov_id, gov));
	}
	return govmap;
}

void automatic_play_until(pompelmous& r, std::map<unsigned int, ai>& ais, int num_turns)
{
	while(!signal_received &&
			r.get_round_number() <= num_turns &&
			r.get_round_number() < r.get_num_turns()) {
		std::map<unsigned int, ai>::iterator ait = ais.find(r.current_civ_id());
		if(ait != ais.end()) {
			ait->second.play();
		}
	}
}

class menu {
	public:
		menu(SDL_Surface* screen_,
				TTF_Font* font_);
		virtual ~menu();
		int run();
	protected:
		virtual void draw_background() = 0;
		virtual void setup_buttons() = 0;
		SDL_Surface* screen;
		TTF_Font* font;
		std::list<button*> buttons;
};

menu::menu(SDL_Surface* screen_,
		TTF_Font* font_)
	: screen(screen_),
	font(font_)
{
}

menu::~menu()
{
	for(std::list<button*>::iterator it = buttons.begin();
			it != buttons.end();
			++it) {
		delete *it;
	}
}

int menu::run()
{
	if(buttons.empty())
		setup_buttons();
	draw_background();
	std::for_each(buttons.begin(), buttons.end(), std::bind2nd(std::mem_fun(&button::draw), screen));
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
	}
	bool running = true;
	while(running) {
		SDL_Event event;
		if(SDL_WaitEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					if(event.key.keysym.sym == SDLK_ESCAPE)
						running = false;
					break;
				case SDL_MOUSEBUTTONUP:
					if(check_button_click(buttons, event))
						running = false;
					break;
				case SDL_QUIT:
					raise(2);
					running = false;
					break;
				default:
					break;
			}
		}
		else {
			fprintf(stderr, "Error on SDL_WaitEvent: %s\n",
					SDL_GetError());
		}
	}
	return 0;
}

class end_screen : public menu {
	public:
		end_screen(SDL_Surface* screen_,
				TTF_Font* font_,
				const pompelmous& r_);
	protected:
		void draw_background();
		void setup_buttons();
	private:
		int quit_button();
		const pompelmous& r;
};

end_screen::end_screen(SDL_Surface* screen_,
		TTF_Font* font_, const pompelmous& r_)
	: menu(screen_, font_),
	r(r_)
{
}

void end_screen::draw_background()
{
	draw_plain_rectangle(screen, 50, 50, screen->w - 100, screen->h - 100,
			color(20, 15, 37));
	char buf[256];
	buf[255] = '\0';
	snprintf(buf, 255, "Game finished");
	draw_text(screen, font, buf, screen->w / 2, 150, 255, 255, 255, true);
	if(r.get_winning_civ() == -1)
		snprintf(buf, 255, "Winner: unknown!");
	else
		snprintf(buf, 255, "Winner: %s", r.civs[r.get_winning_civ()]->civname.c_str());
	draw_text(screen, font, buf, screen->w / 2, 300, 255, 255, 255, true);
	switch(r.get_victory_type()) {
		case victory_score:
			snprintf(buf, 255, "Win by score");
			break;
		case victory_elimination:
			snprintf(buf, 255, "Win by elimination");
			break;
		case victory_domination:
			snprintf(buf, 255, "Win by domination");
			break;
		default:
			snprintf(buf, 255, "Win by something unknown");
			break;
	}
	draw_text(screen, font, buf, screen->w / 2, 400, 255, 255, 255, true);
}

void end_screen::setup_buttons()
{
	plain_button* quit_button = new plain_button(rect(412, 600, 200, 60),
			"Exit", font, color(4, 0, 132),
			color(200, 200, 200), boost::bind(&end_screen::quit_button, this));
	buttons.push_back(quit_button);
}

int end_screen::quit_button()
{
	return 1;
}

void display_end_screen(const pompelmous& r)
{
	end_screen e(screen, font, r);
	e.run();
}

void play_game(pompelmous& r, std::map<unsigned int, ai>& ais)
{
	bool running = true;

	if(use_gui) {
		std::vector<std::string> terrain_files = get_file_list(KINGDOMS_GFXDIR, KINGDOMS_RULESDIR "terrain-gfx.txt");
		std::vector<std::string> unit_files = get_file_list(KINGDOMS_GFXDIR, KINGDOMS_RULESDIR "units-gfx.txt");

		std::vector<const char*> road_images;
		road_images.push_back(KINGDOMS_GFXDIR "road_nw.png");
		road_images.push_back(KINGDOMS_GFXDIR "road_w.png");
		road_images.push_back(KINGDOMS_GFXDIR "road_sw.png");
		road_images.push_back(KINGDOMS_GFXDIR "road_n.png");
		road_images.push_back(KINGDOMS_GFXDIR "road.png");
		road_images.push_back(KINGDOMS_GFXDIR "road_s.png");
		road_images.push_back(KINGDOMS_GFXDIR "road_ne.png");
		road_images.push_back(KINGDOMS_GFXDIR "road_e.png");
		road_images.push_back(KINGDOMS_GFXDIR "road_se.png");
		gui g(1024, 768, screen, r.get_map(), r, terrain_files, unit_files, KINGDOMS_GFXDIR "empty.png", 
				KINGDOMS_GFXDIR "city.png", *font,
				KINGDOMS_GFXDIR "food_icon.png",
				KINGDOMS_GFXDIR "prod_icon.png",
				KINGDOMS_GFXDIR "comm_icon.png",
				KINGDOMS_GFXDIR "irrigation.png",
				KINGDOMS_GFXDIR "mine.png",
				road_images,
				observer ? &ais.find(0)->second : NULL, r.civs[0]);
		g.display();
		g.init_turn();
		if(observer && skip_rounds > 0)
			automatic_play_until(r, ais, skip_rounds);
		r.add_action_listener(&g);
		while(running) {
			if(r.current_civ_id() == (int)r.civs[0]->civ_id) {
				SDL_Event event;
				while(SDL_PollEvent(&event)) {
					switch(event.type) {
						case SDL_KEYDOWN:
						case SDL_MOUSEBUTTONDOWN:
						case SDL_MOUSEBUTTONUP:
							if(g.handle_input(event))
								running = false;
							break;
						case SDL_QUIT:
							running = false;
						default:
							break;
					}
				}
				SDL_Delay(50);
				g.process(50);
			}
			else {
				std::map<unsigned int, ai>::iterator ait = ais.find(r.current_civ_id());
				if(ait != ais.end()) {
					if(ait->second.play()) {
						running = false;
					}
					else {
						if(!observer && r.civs[0]->eliminated()) {
							running = false;
						}
						else {
							g.init_turn();
							if(r.finished()) {
								running = false;
							}
						}
					}
				}
				else {
					running = false;
				}
			}
		}
		if(r.finished())
			display_end_screen(r);
		r.remove_action_listener(&g);
	}
	else {
		automatic_play_until(r, ais, r.get_num_turns());
	}

	for(unsigned int i = 0; i < r.civs.size(); i++) {
		const std::map<unsigned int, int>& m1 = r.civs[i]->get_built_units();
		const std::map<unsigned int, int>& m2 = r.civs[i]->get_lost_units();
		printf("%-20s%-6d points    %4d cities\n%-20s%-6s%-6s\n", r.civs[i]->civname.c_str(),
				r.civs[i]->get_points(), r.civs[i]->cities.size(),
			       	"Unit", "Built", "Lost");
		for(std::map<unsigned int, int>::const_iterator mit = m1.begin();
				mit != m1.end();
				++mit) {
			unsigned int key = mit->first;
			unit_configuration_map::const_iterator uit = r.uconfmap.find(key);
			std::map<unsigned int, int>::const_iterator mit2 = m2.find(mit->first);
			if(uit != r.uconfmap.end()) {
				printf("%-20s%-6d%-6d\n", uit->second.unit_name.c_str(),
						mit->second,
						mit2 == m2.end() ? 0 : mit2->second);
			}
		}
		printf("\n");
	}
}

int run_game(pompelmous& r)
{
	std::map<unsigned int, ai> ais;
	if(observer) {
		ais.insert(std::make_pair(0, ai(r.get_map(), r, r.civs[0])));
		if(ai_debug)
			set_ai_debug_civ(0);
	}
	for(unsigned int i = 1; i < r.civs.size(); i++)
		ais.insert(std::make_pair(i, ai(r.get_map(), r, r.civs[i])));
	play_game(r, ais);
	return 0;
}

class main_menu : public menu {
	public:
		enum main_menu_selection {
			main_menu_start,
			main_menu_load,
			main_menu_quit,
		};

		main_menu(SDL_Surface* screen_,
				TTF_Font* font_);
		main_menu_selection get_selection() const;
	protected:
		void draw_background();
		void setup_buttons();
	private:
		int start_game_button(main_menu_selection s);
		int quit_button();
		main_menu_selection sel;
		SDL_Surface* bg_img;
};

main_menu::main_menu(SDL_Surface* screen_,
		TTF_Font* font_)
	: menu(screen_, font_),
	sel(main_menu::main_menu_quit),
	bg_img(NULL)
{
}

main_menu::main_menu_selection main_menu::get_selection() const
{
	return sel;
}

void main_menu::draw_background()
{
	if(bg_img == NULL) {
		bg_img = sdl_load_image(KINGDOMS_GFXDIR "pergamon.png");
		if(!bg_img) {
			fprintf(stderr, "Unable to load image %s: %s\n", KINGDOMS_GFXDIR "pergamon.png",
					SDL_GetError());
			return;
		}
	}
	if(draw_image(0, 0, bg_img, screen)) {
		fprintf(stderr, "Could not draw background image: %s\n",
				SDL_GetError());
	}
	sel = main_menu_quit;
}

void main_menu::setup_buttons()
{
	sel = main_menu_quit;
	plain_button* start_button = new plain_button(rect(312, 400, 400, 60),
			"Start new game", font, color(40, 30, 10),
			color(200, 200, 200), boost::bind(&main_menu::start_game_button, this, main_menu_start));
	plain_button* load_button = new plain_button(rect(312, 500, 400, 60),
			"Load game", font, color(40, 30, 10),
			color(200, 200, 200), boost::bind(&main_menu::start_game_button, this, main_menu_load));
	plain_button* quit_button = new plain_button(rect(312, 600, 400, 60),
			"Quit", font, color(40, 30, 10),
			color(200, 200, 200), boost::bind(&main_menu::quit_button, this));
	buttons.push_back(start_button);
	buttons.push_back(load_button);
	buttons.push_back(quit_button);
}

int main_menu::start_game_button(main_menu::main_menu_selection s)
{
	sel = s;
	return 1;
}

int main_menu::quit_button()
{
	sel = main_menu_quit;
	return 1;
}

class load_menu : public menu {
	public:
		load_menu(SDL_Surface* screen_,
				TTF_Font* font_) : 
			menu(screen_, font_), 
			load_successful(false)
	{
	}
		pompelmous* get_loaded_game();
	protected:
		void draw_background();
		void setup_buttons();
	private:
		int load_game_button(const std::string& fn);
		int cancel_load_game();
		pompelmous loaded;
		bool load_successful;
};

pompelmous* load_menu::get_loaded_game()
{
	if(load_successful)
		return &loaded;
	else
		return NULL;
}

void load_menu::draw_background()
{
	draw_plain_rectangle(screen, 300, 200, 400, 420, color(220, 150, 37));
}

bool compare_filenames(const boost::filesystem::path& p1,
		const boost::filesystem::path& p2)
{
	return p1.filename() > p2.filename();
}

void load_menu::setup_buttons()
{
	using namespace boost::filesystem;
	path svg_path = path(path_to_saved_games());
	if(!exists(svg_path))
		return;
	directory_iterator end_itr;
	rect fn_rect(320, 220, 360, 30);

	std::vector<path> filenames;
	for (directory_iterator itr(svg_path);
			itr != end_itr;
			++itr) {
		if (is_regular_file(itr->status()))
		{
			filenames.push_back(itr->path());
		}
	}

	std::sort(filenames.begin(), filenames.end(), compare_filenames);

	for(std::vector<path>::const_iterator it = filenames.begin();
			it != filenames.end();
			++it) {
		std::string s(it->string());
		std::string fp(it->filename().string());
		plain_button* load_game_button = new plain_button(fn_rect,
				fp.c_str(), font, color(206, 187, 158),
				color(0, 0, 0),
				boost::bind(&load_menu::load_game_button, 
					this, s));
		buttons.push_back(load_game_button);
		fn_rect.y += 35;
		if(fn_rect.y >= 560)
			break;
	}
	plain_button* exit_load_button = new plain_button(fn_rect,
			"Cancel", font, color(206, 187, 158),
			color(0, 0, 0),
			boost::bind(&load_menu::cancel_load_game, this));
	buttons.push_back(exit_load_button);
}

int load_menu::load_game_button(const std::string& fn)
{
	load_successful = load_game(fn.c_str(), loaded);
	return 1;
}

int load_menu::cancel_load_game()
{
	return 1;
}

int run_loaded_gamedata()
{
	load_menu l(screen, font);
	l.run();
	pompelmous* r = l.get_loaded_game();
	if(r) {
		run_game(*r);
		return 0;
	}
	else {
		return 1;
	}
}

int run_gamedata()
{
	const int map_x = 80;
	const int map_y = 60;
	const int road_moves = 3;
	const unsigned int food_eaten_per_citizen = 2;

	const int num_turns = 400;

	resource_configuration resconf = parse_resource_config(KINGDOMS_RULESDIR "terrain.txt");
	std::vector<civilization*> civs;
	civs = parse_civs_config(KINGDOMS_RULESDIR "civs.txt");
	unit_configuration_map uconfmap = parse_unit_config(KINGDOMS_RULESDIR "units.txt");
	advance_map amap = parse_advance_config(KINGDOMS_RULESDIR "discoveries.txt");
	city_improv_map cimap = parse_city_improv_config(KINGDOMS_RULESDIR "improvs.txt");
	government_map govmap = parse_government_config(KINGDOMS_RULESDIR "governments.txt");
	map m(map_x, map_y, resconf);
	for(unsigned int i = 0; i < civs.size(); i++) {
		civs[i]->set_map(&m);
		civs[i]->set_government(&govmap.begin()->second);
	}
	pompelmous r(uconfmap, amap, cimap, &m, road_moves,
			food_eaten_per_citizen, num_turns);

	std::vector<coord> starting_places = m.get_starting_places(civs.size());
	if(starting_places.size() != civs.size()) {
		printf("Could find only %d starting places (instead of %d).\n",
				starting_places.size(), civs.size());
		if(starting_places.size() < 3) {
			exit(1);
		}
	}
	for(unsigned int i = 0; i < starting_places.size(); i++) {
		// settler
		civs[i]->add_unit(0, starting_places[i].x, starting_places[i].y, 
				(*(r.uconfmap.find(0))).second, road_moves);
		// warrior
		civs[i]->add_unit(2, starting_places[i].x, starting_places[i].y, 
				(*(r.uconfmap.find(2))).second, road_moves);
		r.add_civilization(civs[i]);
	}
	int ret = run_game(r);
	for(unsigned int i = 0; i < civs.size(); i++) {
		delete civs[i];
	}
	return ret;
}

void run_mainmenu()
{
	if(use_gui) {
		screen = SDL_SetVideoMode(1024, 768, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
		if (!screen) {
			fprintf(stderr, "Unable to set %dx%d video: %s\n", 1024, 768, SDL_GetError());
			return;
		}
		font = TTF_OpenFont(KINGDOMS_GFXDIR "DejaVuSans.ttf", 12);
		if(!font) {
			fprintf(stderr, "Could not open font: %s\n", TTF_GetError());
		}
		else {
			main_menu m(screen, font);
			bool running = true;
			while(running) {
				m.run();
				main_menu::main_menu_selection s = m.get_selection();
				switch(s) {
					case main_menu::main_menu_start:
						load = false;
						run_gamedata();
						break;
					case main_menu::main_menu_load:
						load = true;
						run_loaded_gamedata();
						break;
					default:
						running = false;
						break;
				}
			}
			TTF_CloseFont(font);
		}
	}
	else {
		run_gamedata();
	}
}

int main(int argc, char **argv)
{
	int seed = 0;
	int c;
	bool succ = true;
	while((c = getopt(argc, argv, "adloxS:s:")) != -1) {
		switch(c) {
			case 'S':
				skip_rounds = atoi(optarg);
				break;
			case 'd':
				ai_debug = true;
				break;
			case 'l':
				load = true;
				break;
			case 'o':
				observer = true;
				break;
			case 'x':
				use_gui = false;
				break;
			case 's':
				seed = atoi(optarg);
				break;
			case '?':
			default:
				fprintf(stderr, "Unrecognized option: -%c\n",
						optopt);
				succ = false;
				break;
		}
	}
	if(!succ)
		exit(2);
	if(!load) {
		if(seed)
			srand(seed);
		else {
			seed = time(NULL);
			printf("Seed: %d\n", seed);
			srand(seed);
		}
	}
	if(use_gui) {
		int sdl_flags = SDL_INIT_EVERYTHING;
		if (SDL_Init(sdl_flags) < 0) {
			fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
			exit(1);
		}
		if(!IMG_Init(IMG_INIT_PNG)) {
			fprintf(stderr, "Unable to init SDL_image: %s\n", IMG_GetError());
		}
		if(TTF_Init() == -1) {
			fprintf(stderr, "Unable to init SDL_ttf: %s\n", TTF_GetError());
		}
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	}
	else {
		signal(SIGINT, signal_handler);
	}

	signal(SIGSEGV, segv_handler);
	signal(SIGABRT, segv_handler);

	if(!use_gui)
		observer = true;

	try {
		run_mainmenu();
	}
	catch (boost::archive::archive_exception& e) {
		printf("boost::archive::archive_exception: %s (code %d).\n",
				e.what(), e.code);
	}
	catch (std::exception& e) {
		printf("std::exception: %s\n", e.what());
	}
	catch(...) {
		printf("Unknown exception.\n");
	}

	if(use_gui) {
		TTF_Quit();
		SDL_Quit();
	}
	return 0;
}

