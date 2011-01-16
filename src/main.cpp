/*
    Kingdoms, an open source strategy game,
    Copyright (C) 2010, 2011 Antti Salonen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#ifdef __gnu_linux__
#include <execinfo.h>
#endif

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
#include "parse_rules.h"

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

#include "gui-resources.h"
#include "config.h"

static bool signal_received = false;

static bool observer = false;
static bool use_gui = true;
static bool ai_debug = false;
static int skip_rounds = 0;

static int given_seed = 0;

static SDL_Surface* screen = NULL;
static TTF_Font* font = NULL;

#ifdef __gnu_linux__
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
#endif

void signal_handler(int sig)
{
	signal_received = true;
}

void automatic_play_until(pompelmous& r, std::map<unsigned int, ai>& ais, int num_turns)
{
	while(!signal_received &&
			r.get_round_number() <= num_turns && !r.finished()) {
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

class simple_info_screen : public menu {
	public:
		simple_info_screen(SDL_Surface* screen_,
				TTF_Font* font_,
				const pompelmous& r_);
	protected:
		virtual void draw_background();
		virtual void setup_buttons();
		const pompelmous& r;
	private:
		int quit_button();
};

simple_info_screen::simple_info_screen(SDL_Surface* screen_,
		TTF_Font* font_, const pompelmous& r_)
	: menu(screen_, font_),
	r(r_)
{
}

void simple_info_screen::draw_background()
{
	draw_plain_rectangle(screen, 50, 50, screen->w - 100, screen->h - 100,
			color(20, 15, 37));
}

void simple_info_screen::setup_buttons()
{
	plain_button* quit_button = new plain_button(rect(412, 600, 200, 60),
			"Exit", font, color(4, 0, 132),
			color(200, 200, 200), boost::bind(&simple_info_screen::quit_button, this));
	buttons.push_back(quit_button);
}

int simple_info_screen::quit_button()
{
	return 1;
}

class end_screen : public simple_info_screen {
	public:
		end_screen(SDL_Surface* screen_,
				TTF_Font* font_,
				const pompelmous& r_);
	protected:
		void draw_background();
};

end_screen::end_screen(SDL_Surface* screen_,
		TTF_Font* font_, const pompelmous& r_)
	: simple_info_screen(screen_, font_, r_)
{
}

void end_screen::draw_background()
{
	simple_info_screen::draw_background();
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

class score_screen : public simple_info_screen {
	public:
		score_screen(SDL_Surface* screen_,
				TTF_Font* font_,
				const pompelmous& r_);
	protected:
		void draw_background();
};

score_screen::score_screen(SDL_Surface* screen_,
		TTF_Font* font_, const pompelmous& r_)
	: simple_info_screen(screen_, font_, r_)
{
}

bool compare_score(const civilization* c1, const civilization* c2)
{
	return c1->get_points() > c2->get_points();
}

void score_screen::draw_background()
{
	simple_info_screen::draw_background();
	char buf[256];
	buf[255] = '\0';
	std::vector<civilization*> civs(r.civs);
	std::sort(civs.begin(), civs.end(), compare_score);
	int yp = 100;
	for(std::vector<civilization*>::const_iterator it = civs.begin();
			it != civs.end();
			++it) {
		draw_text(screen, font, (*it)->civname.c_str(), 70, yp, 255, 255, 255, false);
		snprintf(buf, 255, "%d", (*it)->get_points());
		draw_text(screen, font, buf, screen->w - 150, yp, 255, 255, 255, false);
		yp += 32;
		if(yp > 580)
			break;
	}
}

void display_score_screen(const pompelmous& r)
{
	score_screen e(screen, font, r);
	e.run();
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
		gui_resource_files grr;
		fetch_gui_resource_files(&grr);
		gui g(screen, r.get_map(), r, grr, *font,
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
						else if(r.current_civ_id() == (int)r.civs[0]->civ_id) {
							g.init_turn();
							if(r.finished()) {
								running = false;
							}
							else {
								if(r.get_round_number() % 4 == 0) {
									printf("Auto-saving.\n");
									save_game("auto", r);
								}
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
		else
			display_score_screen(r);
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
	directory_iterator end_itr;
	rect fn_rect(320, 220, 360, 30);

	if(exists(svg_path)) {
		std::string save_file_ext = SAVE_FILE_EXTENSION;
		std::vector<path> filenames;
		for (directory_iterator itr(svg_path);
				itr != end_itr;
				++itr) {
			if(is_regular_file(itr->status())) {
				if(boost::algorithm::to_lower_copy(itr->path().extension().string()) 
						== save_file_ext)
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
	}

	fn_rect.y = 575;
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
	const int num_turns = 300;

	resource_configuration resconf = parse_terrain_config(KINGDOMS_RULESDIR "terrain.txt");
	std::vector<civilization*> civs;
	civs = parse_civs_config(KINGDOMS_RULESDIR "civs.txt");
	unit_configuration_map uconfmap = parse_unit_config(KINGDOMS_RULESDIR "units.txt");
	advance_map amap = parse_advance_config(KINGDOMS_RULESDIR "discoveries.txt");
	city_improv_map cimap = parse_city_improv_config(KINGDOMS_RULESDIR "improvs.txt");
	government_map govmap = parse_government_config(KINGDOMS_RULESDIR "governments.txt");
	resource_map rmap = parse_resource_config(KINGDOMS_RULESDIR "resources.txt");
	map m(map_x, map_y, resconf, rmap);
	m.create();
	for(unsigned int i = 0; i < civs.size(); i++) {
		civs[i]->set_map(&m);
		civs[i]->set_government(&govmap.begin()->second);
	}
	pompelmous r(uconfmap, amap, cimap, &m, road_moves,
			food_eaten_per_citizen, num_turns);

	std::vector<coord> starting_places = m.random_starting_places(civs.size());
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

void setup_seed()
{
	if(given_seed) {
		srand(given_seed);
		printf("Seed (given): %d\n", given_seed);
	}
	else {
		int seed = time(NULL);
		printf("Seed: %d\n", seed);
		srand(seed);
	}
}

void run_mainmenu()
{
	if(use_gui) {
		screen = SDL_SetVideoMode(1024, 768, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
		if (!screen) {
			fprintf(stderr, "Unable to set %dx%d video: %s\n", 1024, 768, SDL_GetError());
			return;
		}
		SDL_WM_SetCaption("Kingdoms", NULL);
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
						setup_seed();
						run_gamedata();
						break;
					case main_menu::main_menu_load:
						setup_seed();
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
		setup_seed();
		run_gamedata();
	}
}

int main(int argc, char **argv)
{
	int c;
	bool succ = true;
	while((c = getopt(argc, argv, "adoxS:s:")) != -1) {
		switch(c) {
			case 'S':
				skip_rounds = atoi(optarg);
				break;
			case 'd':
				ai_debug = true;
				break;
			case 'o':
				observer = true;
				break;
			case 'x':
				use_gui = false;
				break;
			case 's':
				given_seed = atoi(optarg);
				break;
			case '?':
			default:
				fprintf(stderr, "Unrecognized option: -%c\n",
						optopt);
				succ = false;
				break;
		}
	}
	if(!succ) {
		exit(2);
	}

	if(use_gui) {
		if(sdl_init_all())
			exit(1);
	}
	else {
		signal(SIGINT, signal_handler);
	}

#ifdef __gnu_linux__
	signal(SIGSEGV, segv_handler);
	signal(SIGABRT, segv_handler);
#endif

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

