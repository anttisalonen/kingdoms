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
#include <string>

#include <boost/bind/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/algorithm/string.hpp>

#include "filesystem.h"
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
#include "paths.h"

static bool signal_received = false;

static bool observer = false;
static bool use_gui = true;
static bool ai_debug = false;
static int skip_rounds = 0;

static int given_seed = 0;

static SDL_Surface* screen = NULL;
static TTF_Font* font = NULL;

static std::string ruleset_name;

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

void automatic_play_until(pompelmous& r, std::map<unsigned int, ai*>& ais, int num_turns)
{
	while(!signal_received &&
			r.get_round_number() <= num_turns && !r.finished()) {
		std::map<unsigned int, ai*>::iterator ait = ais.find(r.current_civ_id());
		if(ait != ais.end()) {
			ait->second->play();
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
					if(event.button.button == SDL_BUTTON_LEFT) {
						if(check_button_click(buttons, event))
							running = false;
					}
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
		if((*it)->is_minor_civ())
			continue;
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

void play_game(pompelmous& r, std::map<unsigned int, ai*>& ais,
		unsigned int own_civ_id)
{
	bool running = true;
	unsigned int own_civ_index = 0;
	for(unsigned int i = 0; i < r.civs.size(); i++) {
		if(r.civs[i]->civ_id == own_civ_id) {
			own_civ_index = i;
			break;
		}
	}

	if(use_gui) {
		gui_resource_files grr;
		fetch_gui_resource_files(ruleset_name, &grr);
		gui g(screen, r.get_map(), r, grr, *font,
				observer ? ais.find(own_civ_id)->second : NULL, r.civs[own_civ_index],
				ruleset_name);
		g.display();
		g.init_turn();
		if(observer && skip_rounds > 0)
			automatic_play_until(r, ais, skip_rounds);
		r.add_action_listener(&g);
		while(running) {
			if(r.current_civ_id() == (int)own_civ_id) {
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
				if(g.process(50)) {
					running = false;
				}
			}
			else {
				std::map<unsigned int, ai*>::iterator ait = ais.find(r.current_civ_id());
				if(ait != ais.end()) {
					if(ait->second->play()) {
						running = false;
					}
					else {
						if(!observer && r.civs[own_civ_index]->eliminated()) {
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
									save_game("auto", ruleset_name, r,
											own_civ_id);
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
		if(r.civs[i]->is_minor_civ())
			continue;
		const std::map<unsigned int, int>& m1 = r.civs[i]->get_built_units();
		const std::map<unsigned int, int>& m2 = r.civs[i]->get_lost_units();
		printf("%-20s%-6d points    %4lu cities\n%-20s%-6s%-6s\n", r.civs[i]->civname.c_str(),
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

int run_game(pompelmous& r, unsigned int own_civ_id)
{
	std::map<unsigned int, ai*> ais;
	if(observer && ai_debug) {
			set_ai_debug_civ(own_civ_id);
	}
	for(unsigned int i = 0; i < r.civs.size(); i++) {
		if(r.civs[i]->civ_id == own_civ_id && !observer)
			continue;
		ai* a = new ai(r.get_map(), r, r.civs[i]);
		std::pair<std::map<unsigned int, ai*>::iterator, bool> res =
			ais.insert(std::make_pair(i, a));
		if(!r.civs[i]->is_minor_civ())
			r.add_diplomat(i, res.first->second);
	}
	play_game(r, ais, own_civ_id);
	for(std::map<unsigned int, ai*>::iterator it = ais.begin();
			it != ais.end();
			++it) {
		delete it->second;
	}
	return 0;
}

class main_menu : public menu {
	public:
		enum main_menu_selection {
			main_menu_start,
			main_menu_load,
			main_menu_map,
			main_menu_quit,
		};

		main_menu(SDL_Surface* screen_,
				TTF_Font* font_);
		~main_menu();
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

main_menu::~main_menu()
{
	SDL_FreeSurface(bg_img);
	bg_img = NULL;
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
	plain_button* load_button = new plain_button(rect(312, 480, 400, 60),
			"Load game", font, color(40, 30, 10),
			color(200, 200, 200), boost::bind(&main_menu::start_game_button, this, main_menu_load));
	plain_button* map_button = new plain_button(rect(312, 560, 400, 60),
			"Load map", font, color(40, 30, 10),
			color(200, 200, 200), boost::bind(&main_menu::start_game_button, this, main_menu_map));
	plain_button* quit_button = new plain_button(rect(312, 640, 400, 60),
			"Quit", font, color(40, 30, 10),
			color(200, 200, 200), boost::bind(&main_menu::quit_button, this));
	buttons.push_back(start_button);
	buttons.push_back(load_button);
	buttons.push_back(map_button);
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

class game_configuration_window {
	public:
		game_configuration_window(SDL_Surface* screen_,
				TTF_Font* font_, bool create_map, const std::vector<civilization*>& civs) :
			bg(sdl_load_image(get_beige_bg_file().c_str())),
			w(screen_, *font_, rect(0, 0, screen->w, screen->h),
					bg),
			map_size(2)
	{
		int start_x = screen->w / 2 - 300;
		int start_y = screen->h / 2 - 200;
		w.set_text_color(color(255, 255, 255));
		w.set_button_color(color(128, 40, 40));
		w.add_key_handler(SDLK_ESCAPE, widget_close);
		w.set_show_focus(false);
		w.add_button(screen->w / 2 - 40, screen->h - 40, 80, 24, "Go",
				boost::bind(&game_configuration_window::go,
					this, boost::lambda::_1));
		if(create_map) {
			std::vector<std::pair<rect, std::string> > map_size_buttons;
			map_size_buttons.push_back(std::make_pair(rect(start_x + 100, start_y + 120, 16, 16),
						"0"));
			map_size_buttons.push_back(std::make_pair(rect(start_x + 100, start_y + 140, 16, 16),
						"1"));
			map_size_buttons.push_back(std::make_pair(rect(start_x + 100, start_y + 160, 16, 16),
						"2"));
			map_size_buttons.push_back(std::make_pair(rect(start_x + 100, start_y + 180, 16, 16),
						"3"));
			map_size_buttons.push_back(std::make_pair(rect(start_x + 100, start_y + 200, 16, 16),
						"4"));
			w.add_label(start_x + 100, start_y + 100, 120, 16, "Map size");
			w.add_label(start_x + 120, start_y + 120, 100, 16, "Tiny");
			w.add_label(start_x + 120, start_y + 140, 100, 16, "Small");
			w.add_label(start_x + 120, start_y + 160, 100, 16, "Medium");
			w.add_label(start_x + 120, start_y + 180, 100, 16, "Large");
			w.add_label(start_x + 120, start_y + 200, 100, 16, "Huge");
			w.add_radio_button_set("Map size buttons", map_size_buttons, 2);
		}
		std::vector<std::string> civnames;
		for(unsigned int i = 0; i < civs.size(); i++)
			civnames.push_back(civs[i]->civname);
		w.add_label(start_x + 250, start_y + 80, 100, 16, "Your civilization");
		w.add_combo_box(start_x + 250, start_y + 100, 100, 16, "My civilization",
				civnames, 8);
	}
		~game_configuration_window()
		{
			SDL_FreeSurface(bg);
			bg = NULL;
		}
		int run();
		int get_map_size() const { return map_size; }
		const std::string& get_own_civ_name() const { return chosen_civ; }
	protected:
		void draw_background();
		void setup_buttons();
	private:
		int go(const widget_window* w);
		SDL_Surface* bg;
		widget_window w;
		int map_size;
		std::string chosen_civ;
};

int game_configuration_window::run()
{
	w.draw_window();
	bool running = true;
	int ret = 0;
	while(running) {
		SDL_Event event;
		if(SDL_WaitEvent(&event)) {
			if((event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
					|| (event.type == SDL_QUIT)) {
				ret = 1;
				running = false;
			}
			else if(w.handle_window_input(event))
				running = false;
			else if(event.type == SDL_MOUSEBUTTONUP)
				w.draw_window();
		}
		else {
			fprintf(stderr, "Error on SDL_WaitEvent: %s\n",
					SDL_GetError());
		}
	}
	for(std::list<widget*>::const_iterator it = w.widgets.begin();
			it != w.widgets.end();
			++it) {
		if((*it)->get_name() == std::string("Map size buttons")) {
			std::string s = (*it)->get_data();
			if(!s.empty()) {
				std::stringstream(s) >> map_size;
			}
		}
		if((*it)->get_name() == std::string("My civilization")) {
			chosen_civ = (*it)->get_data();
		}
	}
	return ret;
}

int game_configuration_window::go(const widget_window* w)
{
	return 1;
}

class load_menu : public menu {
	public:
		load_menu(SDL_Surface* screen_,
				TTF_Font* font_,
				bool load_map_) : 
			menu(screen_, font_), 
			load_successful(false),
			want_map(load_map_)
	{
	}
		pompelmous* get_loaded_game();
		map* get_loaded_map();
		unsigned int get_own_civ_id() const;
	protected:
		void draw_background();
		void setup_buttons();
	private:
		int load_game_button(const std::string& fn);
		int cancel_load_game();
		pompelmous loaded;
		map loaded_map;
		bool load_successful;
		bool want_map;
		unsigned int own_civ_id;
};

pompelmous* load_menu::get_loaded_game()
{
	if(load_successful && !want_map)
		return &loaded;
	else
		return NULL;
}

map* load_menu::get_loaded_map()
{
	if(load_successful && want_map)
		return &loaded_map;
	else
		return NULL;
}

unsigned int load_menu::get_own_civ_id() const
{
	return own_civ_id;
}

void load_menu::draw_background()
{
	draw_plain_rectangle(screen, 300, 200, 400, 420, color(220, 150, 37));
}

void load_menu::setup_buttons()
{
	std::vector<boost::filesystem::path> filenames;
	std::string color_marking_path;
	if(want_map) {
		std::string saved_maps_path = path_to_saved_maps(ruleset_name);
		filenames = get_files_in_directory(path_to_saved_maps(ruleset_name),
			MAP_FILE_EXTENSION);
		std::string preinstalled_path = get_preinstalled_maps_path(ruleset_name);
		if(preinstalled_path != saved_maps_path) {
			color_marking_path = preinstalled_path;
			std::vector<boost::filesystem::path> preinstalled =
				get_files_in_directory(get_preinstalled_maps_path(ruleset_name),
						MAP_FILE_EXTENSION);
			filenames.insert(filenames.end(), preinstalled.begin(), preinstalled.end());
		}
	}
	else {
		filenames = get_files_in_directory(path_to_saved_games(ruleset_name),
			SAVE_FILE_EXTENSION);
		std::reverse(filenames.begin(), filenames.end());
	}
	rect fn_rect(320, 220, 360, 30);

	for(std::vector<boost::filesystem::path>::const_iterator it = filenames.begin();
			it != filenames.end();
			++it) {
		std::string s(it->string());
		std::string fp(it->stem().string());
		std::string fpath(it->parent_path().string());
		plain_button* load_game_button = new plain_button(fn_rect,
				fp.c_str(), font,
				!color_marking_path.empty() && fpath == color_marking_path ? 
				color(231, 173, 86) : 
				color(206, 187, 158),
				color(0, 0, 0),
				boost::bind(&load_menu::load_game_button, 
					this, s));
		buttons.push_back(load_game_button);
		fn_rect.y += 35;
		if(fn_rect.y >= 560)
			break;
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
	own_civ_id = 0;
	load_successful = want_map ?
		load_map(fn.c_str(), loaded_map) :
		load_game(fn.c_str(), loaded, own_civ_id);
	return 1;
}

int load_menu::cancel_load_game()
{
	return 1;
}

int run_with_map(map& m, std::vector<civilization*>& civs, int own_civ_id)
{
	const int road_moves = 3;
	const unsigned int food_eaten_per_citizen = 2;
	const int num_turns = 400;
	const unsigned int anarchy_period = 1;
	const unsigned int num_barbarians = 100;
	bool self_included = false;
	if(own_civ_id == -1)
		own_civ_id = 0;

	unit_configuration_map uconfmap;
	advance_map amap;
	city_improv_map cimap;
	government_map govmap;
	get_configuration(ruleset_name, NULL, &uconfmap, &amap, &cimap, NULL, &govmap, NULL);

	pompelmous r(uconfmap, amap, cimap, govmap, &m, road_moves,
			food_eaten_per_citizen, anarchy_period, num_turns);

	for(unsigned int i = 0; i < civs.size(); i++) {
		civs[i]->set_map(&m);
		civs[i]->set_government(&govmap.begin()->second);
		civs[i]->set_city_improvement_map(&r.cimap);
	}

	std::map<int, coord> starting_places = m.get_starting_places();
	if(starting_places.size() < 3) {
		starting_places.clear();
		bool found = false;
		int min_distance = 10;
		while(!found) {
			std::vector<coord> starting_places_vect = m.random_starting_places(civs.size(),
					true, min_distance);
			if(starting_places_vect.size() != civs.size()) {
				printf("Could find only %lu starting places (instead of %lu).\n",
						starting_places_vect.size(), civs.size());
			}
			if(starting_places_vect.size() >= 3) {
				for(unsigned int i = 0; i < starting_places_vect.size(); i++) {
					starting_places[i] = starting_places_vect[i];
				}
				found = true;
			}
			if(min_distance > 2)
				min_distance--;
			else
				return 1;
		}
	}
	for(std::map<int, coord>::const_iterator it = starting_places.begin();
			it != starting_places.end();
			++it) {
		if(own_civ_id == it->first) {
			self_included = true;
		}
	}
	// remap civ ids in order to have correct civ ids in pompelmous
	// TODO: rework pompelmous to use std::map<unsigned int, civilization*>
	// instead of std::vector<unsigned int>
	int assigned_civ_id = 0;
	for(std::map<int, coord>::const_iterator it = starting_places.begin();
			it != starting_places.end();
			++it) {
		int i = it->first;
		if(!self_included) {
			i = own_civ_id;
			own_civ_id = assigned_civ_id;
			self_included = true;
		}
		const coord& c = it->second;
		// rename the civ id because of civ indexing in pompelmous
		civs[i]->civ_id = assigned_civ_id;
		// settler
		civs[i]->add_unit(0, c.x, c.y, 
				(*(r.uconfmap.find(0))).second, road_moves);
		// warrior
		civs[i]->add_unit(2, c.x, c.y, 
				(*(r.uconfmap.find(2))).second, road_moves);
		r.add_civilization(civs[i]);
		assigned_civ_id++;
	}

	std::vector<std::string> barbarian_names;
	for(int i = 0; i < 20; i++) {
		std::stringstream s;
		s << "Barbarian Dwelling " << (i + 1);
		barbarian_names.push_back(s.str());
	}
	std::vector<civilization*> barbarians;
	{
		std::vector<coord> barbarian_spots = m.random_starting_places(num_barbarians,
				false, 2);
		int added_barbarians = 0;
		for(std::vector<coord>::iterator it = barbarian_spots.begin();
				it != barbarian_spots.end();
				++it) {
			bool add_this = true;
			for(std::map<int, coord>::const_iterator it2 = starting_places.begin();
					it2 != starting_places.end();
					++it2) {
				if(m.manhattan_distance(it->x, it->y,
							it2->second.x, it2->second.y) < 4) {
					add_this = false;
					break;
				}
			}
			if(add_this) {
				added_barbarians++;
				int id = r.civs.size();
				civilization* barb = new civilization("Barbarians",
						id, color(20, 20, 20),
						&m, barbarian_names.begin(),
						barbarian_names.end(),
						&r.cimap,
						&govmap.begin()->second, true);
				barbarians.push_back(barb);
				// warrior
				barb->add_unit(2, it->x, it->y, 
						(*(r.uconfmap.find(2))).second, road_moves);
				r.add_civilization(barb);
			}
		}
		printf("Added %d barbarian tribes.\n",
				added_barbarians);
	}

	int ret = run_game(r, own_civ_id);
	for(unsigned int i = 0; i < barbarians.size(); i++) {
		delete barbarians[i];
	}
	return ret;
}

int enter_game_configuration_window(map* m, std::vector<civilization*>& civs)
{
	bool create_map = m == NULL;
	game_configuration_window w(screen, font, create_map, civs);
	if(w.run())
		return 0;
	resource_configuration resconf;
	resource_map rmap;
	get_configuration(ruleset_name, NULL, NULL, NULL, NULL, &resconf, NULL, &rmap);
	if(create_map) {
		int map_x, map_y;
		map_x = w.get_map_size() * 20 + 60;
		map_y = map_x - 20;
		m = new map(map_x, map_y, resconf, rmap);
		m->create();
	}
	int own_civ_id = 0;
	for(unsigned int i = 0; i < civs.size(); i++) {
		if(civs[i]->civname == w.get_own_civ_name()) {
			own_civ_id = i;
			break;
		}
	}
	int ret = run_with_map(*m, civs, own_civ_id);
	if(create_map)
		delete m;
	return ret;
}

int run_gamedata(std::vector<civilization*>& civs)
{
	const int map_x = 80;
	const int map_y = 60;

	resource_configuration resconf;
	resource_map rmap;
	get_configuration(ruleset_name, NULL, NULL, NULL, NULL, &resconf, NULL, &rmap);
	map m(map_x, map_y, resconf, rmap);
	m.create();
	return run_with_map(m, civs, -1);
}

int run_loaded_gamedata(bool want_map, std::vector<civilization*>& civs)
{
	load_menu l(screen, font, want_map);
	l.run();
	if(!want_map) {
		pompelmous* r = l.get_loaded_game();
		if(r) {
			run_game(*r, l.get_own_civ_id());
			return 0;
		}
		else {
			return 1;
		}
	}
	else {
		map* m = l.get_loaded_map();
		if(m) {
			// TODO: currently works only if none or all civ starting places set
			enter_game_configuration_window(m, civs);
			return 0;
		}
		else {
			return 1;
		}
	}
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

void run_mainmenu(bool fullscreen)
{
	if(use_gui) {
		int w, h;
		if(fullscreen) {
			const SDL_VideoInfo* vi = SDL_GetVideoInfo();
			if(!vi) {
				fprintf(stderr, "Unable to retrieve video information: %s\n", SDL_GetError());
				return;
			}
			w = vi->current_w;
			h = vi->current_h;
			screen = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN);
		} else {
			screen = SDL_SetVideoMode(1024, 768, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
		}

		if (!screen) {
			fprintf(stderr, "Unable to set %dx%d video: %s\n", w, h, SDL_GetError());
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
				std::vector<civilization*> civs;
				get_configuration(ruleset_name, &civs, NULL, NULL, NULL, NULL, NULL, NULL);
				switch(s) {
					case main_menu::main_menu_start:
						setup_seed();
						enter_game_configuration_window(NULL, civs);
						break;
					case main_menu::main_menu_load:
						setup_seed();
						run_loaded_gamedata(false, civs);
						break;
					case main_menu::main_menu_map:
						setup_seed();
						run_loaded_gamedata(true, civs);
						break;
					default:
						running = false;
						break;
				}
				for(unsigned int i = 0; i < civs.size(); i++) {
					delete civs[i];
				}
			}
			TTF_CloseFont(font);
		}
	}
	else {
		std::vector<civilization*> civs;
		get_configuration(ruleset_name, &civs, NULL, NULL, NULL, NULL, NULL, NULL);
		setup_seed();
		run_gamedata(civs);
		for(unsigned int i = 0; i < civs.size(); i++) {
			delete civs[i];
		}
	}
}

void usage(const char* pn)
{
	fprintf(stderr, "Usage: %s [-S <rounds>] [-d] [-o] [-x] [-s <seed>] [-r <ruleset name>]\n\n",
			pn);
	fprintf(stderr, "\t-S rounds:  skip a number of rounds\n");
	fprintf(stderr, "\t-d:         AI debug mode\n");
	fprintf(stderr, "\t-o:         observer mode\n");
	fprintf(stderr, "\t-x:         disable GUI\n");
	fprintf(stderr, "\t-s seed:    set random seed\n");
	fprintf(stderr, "\t-r ruleset: use custom ruleset\n");
	fprintf(stderr, "\t-f:         run fullscreen\n");
}

int main(int argc, char **argv)
{
	int c;
	bool succ = true;
	bool fullscreen = false;
	ruleset_name = "default";

	// work around some locale issues with boost and g++ that result in
	// a crash ("locale::facet::_S_create_c_locale name not valid") when
	// trying to load a game.
	if(!getenv("LC_ALL")) {
		if(setenv("LC_ALL", "C", 0)) {
			perror("setenv");
		}
	}

	while((c = getopt(argc, argv, "adoxS:s:r:hf")) != -1) {
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
			case 'r':
				ruleset_name = std::string(optarg);
				break;
			case 'f':
				fullscreen = true;
				break;
			case 'h':
				usage(argv[0]);
				exit(2);
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
		run_mainmenu(fullscreen);
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

