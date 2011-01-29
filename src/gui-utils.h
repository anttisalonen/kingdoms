#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include <vector>
#include <stdlib.h>

#include <boost/function.hpp>

#include "SDL/SDL.h"

#include "color.h"
#include "sdl-utils.h"
#include "rect.h"
#include "pompelmous.h"

struct tileset {
	tileset(int w, int h);
	std::vector<SDL_Surface*> textures;
	SDL_Surface* irrigation_overlay;
	SDL_Surface* mine_overlay;
	SDL_Surface* road_overlays[9];
	SDL_Surface* river_overlays[5];
	const int tile_w;
	const int tile_h;
};

typedef std::map<std::pair<int, color>, SDL_Surface*> UnitImageMap;

struct gui_resources {
	gui_resources(const TTF_Font& f, int tile_w, int tile_h,
			SDL_Surface* food_, SDL_Surface* prod_,
			SDL_Surface* comm_);
	~gui_resources();
	tileset terrains;
	std::vector<SDL_Surface*> plain_unit_images;
	UnitImageMap unit_images;
	const TTF_Font& font;
	std::vector<SDL_Surface*> city_images;
	std::map<unsigned int, SDL_Surface*> resource_images;
	SDL_Surface* food_icon;
	SDL_Surface* prod_icon;
	SDL_Surface* comm_icon;
	SDL_Surface* get_unit_tile(const unit& u, const color& c);
};

class gui_data {
	public:
		gui_data(map& mm, pompelmous& rr);
		map& m;
		pompelmous& r;
};

class widget {
	public:
		widget(const std::string& name_, const rect& dim_);
		virtual ~widget() { }
		virtual int draw(SDL_Surface* screen) = 0;
		virtual int handle_input(const SDL_Event& ev) = 0;
		virtual const std::string& get_name() const;
		virtual std::string get_data() const;
	protected:
		std::string name;
	public:
		rect dim;
};

class button : public widget {
	public:
		button(const std::string& name_, const rect& dim_, boost::function<int()> onclick_);
		virtual ~button() { }
		virtual int handle_input(const SDL_Event& ev);
		boost::function<int()>onclick;
	protected:
		int draw_surface(SDL_Surface* screen, SDL_Surface* surf);
};

class texture_button : public button {
	public:
		texture_button(const std::string& name_, const rect& dim_, SDL_Surface* surf_, 
				boost::function<int()> onclick_);
		~texture_button();
		int draw(SDL_Surface* screen);
	private:
		SDL_Surface* surf;
};

class plain_button : public button {
	public:
		plain_button(const rect& dim_, const char* text, const TTF_Font* font,
				const color& bg_col, const color& text_col, 
				boost::function<int()> onclick_);
		~plain_button();
		int draw(SDL_Surface* screen);
	private:
		SDL_Surface* surf;
};

class window {
	public:
		window(SDL_Surface* screen_);
		virtual ~window() { } 
		int draw();
		int process(int ms);
		void init_turn();
		int handle_input(const SDL_Event& ev);
		void add_subwindow(window* w);
	protected:
		virtual int handle_window_input(const SDL_Event& ev) = 0;
		virtual int draw_window() = 0;
		virtual int process_window(int ms) { return 0; }
		virtual void init_window_turn() { }
		int num_subwindows() const;
		SDL_Surface* screen;
	private:
		std::list<window*> subwindows;
};

class kingdoms_window : public window {
	public:
		kingdoms_window(SDL_Surface* screen_, gui_data& data_,
				gui_resources& res_);
	protected:
		gui_data& data;
		gui_resources& res;
};

class textbox : public widget {
	public:
		textbox(const TTF_Font* font_, const rect& dim_,
				const color& bg_color_, const color& text_color_,
				const std::string& name_,
				const std::string& default_string_);
		int draw(SDL_Surface* screen);
		virtual int handle_input(const SDL_Event& ev);
		const std::string& get_text() const;
		virtual std::string get_data() const;
	protected:
		std::string text;
	private:
		const TTF_Font* font;
		color bg_color;
		color text_color;
};

class numeric_textbox : public textbox {
	public:
		numeric_textbox(const TTF_Font* font_, const rect& dim_,
				const color& bg_color_, const color& text_color_,
				const std::string& name_,
				int default_num);
		int handle_input(const SDL_Event& ev);
		int get_numeric_value() const;
};

class checkbox : public button {
	public:
		checkbox(const rect& dim_,
				const color& box_col, const color& frame_col,
				const std::string& name_, bool default_checked);
		~checkbox();
		int draw(SDL_Surface* screen);
		bool get_checked() const;
		std::string get_data() const;
	private:
		int on_click();
		SDL_Surface* surf;
		bool checked;
		color frame_col;
};

class radio_button : public button {
	public:
		radio_button(const rect& dim_,
				const color& circle_col_,
				const std::string& name_,
				const std::string& option_name_,
			       	bool chosen_,
				boost::function<int()> on_click_);
		~radio_button();
		void set_chosen(bool c);
		int draw(SDL_Surface* screen);
		bool get_chosen() const;
		std::string get_data() const;
	private:
		int on_click_handler();
		SDL_Surface* surf1;
		SDL_Surface* surf2;
		std::string option_name;
		bool chosen;
		boost::function<int()> radio_onclick;
};

class combo_box : public widget {
	public:
		combo_box(const rect& dim_, const std::string& name_,
				const std::vector<std::string>& items_,
				const TTF_Font* font_,
				const color& bg_col_, const color& text_col_,
				unsigned int max_items_expanded_);
		~combo_box();
		int draw(SDL_Surface* screen);
		virtual int handle_input(const SDL_Event& ev);
		std::string get_data() const;
	private:
		void setup_surface();
		void draw_arrow(int x, int y, bool up);
		void draw_text_box(int x, int y, int ind);
		int orig_h;
		std::vector<std::string> items;
		const TTF_Font* font;
		color bg_col;
		color text_col;
		SDL_Surface* surf;
		int expanded_index;
		unsigned int chosen_index;
		unsigned int max_items_expanded;
		unsigned int num_items_expanded;
};

class input_text_window : public kingdoms_window {
	public:
		input_text_window(SDL_Surface* screen_, 
				gui_data& data_,
				gui_resources& res_,
				const rect& rect_,
				const std::string& info_string_,
				const std::string& default_string,
				const color& bg_color_,
				const color& button_color,
				const color& button_text_color_,
				boost::function<int(const std::string&)> on_ok_,
				boost::function<int(const std::string&)> on_cancel_);
	protected:
		virtual int handle_window_input(const SDL_Event& ev);
		virtual int draw_window();
	private:
		int on_ok();
		int on_cancel();
		rect dims;
		std::string info_string;
		std::list<button*> buttons;
		color bg_color;
		color text_color;
		boost::function<int(const std::string&)> on_ok_func;
		boost::function<int(const std::string&)> on_cancel_func;
		textbox tb;
};

class widget_window : public window {
	public:
		widget_window(SDL_Surface* screen_,
				const TTF_Font& font_,
				const rect& rect_,
				const color& bg_color_);
		widget_window(SDL_Surface* screen_,
				const TTF_Font& font_,
				const rect& rect_,
				const SDL_Surface* bg_img_);
		~widget_window();
		int handle_window_input(const SDL_Event& ev);
		int draw_window();
		void set_text_color(const color& c);
		void set_button_color(const color& c);
		void set_show_focus(bool f);
		void add_label(int x, int y, int w, int h, const std::string& text);
		void add_numeric_textbox(int x, int y, const std::string& text, int val);
		void add_checkbox(int x, int y, int w, int h,
				const std::string& text, bool checked);
		void add_button(int x, int y, int w, int h, const std::string& text,
				boost::function<int(const widget_window*)> cb);
		void add_key_handler(SDLKey k, boost::function<int(const widget_window*)> cb);
		void add_radio_button_set(const std::string& name,
				const std::vector<std::pair<rect, std::string> >& buttons,
				unsigned int chosen_index);
		void add_combo_box(int x, int y, int w, int h,
				const std::string& name,
				const std::vector<std::string>& items,
				unsigned int num_items_expanded);
		std::list<widget*> widgets;
	private:
		int on_button_click(boost::function<int(const widget_window*)> cb);
		void set_focus_widget(int x, int y);
		int on_radio_button_click(const std::string& name, const std::string& option_name);
		const TTF_Font& font;
		rect dim;
		color bg_color;
		color text_color;
		color button_color;
		widget* focus_widget;
		const SDL_Surface* bg_img;
		bool show_focus;
		std::map<SDLKey, boost::function<int(const widget_window*)> > key_handlers;
		std::map<std::string, std::vector<radio_button*> > radio_button_groups;
};

int widget_close(const widget_window* w);
int empty_click_handler(const std::string& s);

int draw_rect(int x, int y, int w, int h, const color& c, 
		int border_width, SDL_Surface* screen);
int draw_image(int x, int y, const SDL_Surface* tile, SDL_Surface* screen);
int draw_terrain_tile(int x, int y, int xpos, int ypos, bool shade,
		const map& m, 
		const tileset& terrains,
		const std::map<unsigned int, SDL_Surface*>& resource_images,
		bool draw_improvements,
		bool draw_resources,
		const std::set<unsigned int>* researched_advances,
		SDL_Surface* screen);
SDL_Surface* make_label(const char* text, const TTF_Font* font, int w, int h, const color& bg_col, const color& text_col);
int check_button_click(const std::list<button*>& buttons,
		const SDL_Event& ev);

#endif
