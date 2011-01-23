#ifndef GAME_WINDOW_H
#define GAME_WINDOW_H

#include "main_window.h"

class game_window : public main_window {
	public:
		game_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_,
				ai* ai_, civilization* myciv_,
				const std::string& ruleset_name_);
		~game_window();
		int process(int ms);
		int handle_window_input(const SDL_Event& ev);
		void init_turn();
		void handle_action(const visible_move_action& a);
	protected:
		char fog_on_tile(int x, int y) const;
		bool city_info_available(const city& c) const;
		bool can_draw_unit(const unit* u) const;
		const std::set<unsigned int>* discovered_advances() const;
		void post_draw();
		void draw_sidebar();
	private:
		void get_next_free_unit();
		int draw_civ_info() const;
		int draw_unit_info() const;
		int draw_eot() const;
		int clear_sidebar() const;
		int handle_mousemotion(int x, int y);
		void numpad_to_move(SDLKey k, int* chx, int* chy) const;
		int handle_civ_messages(std::list<msg>* messages);
		bool try_move_unit(unit* u, int chx, int chy);
		action input_to_action(const SDL_Event& ev);
		action observer_action(const SDL_Event& ev);
		void handle_input_gui_mod(const SDL_Event& ev, city** c);
		void handle_successful_action(const action& a, city** c);
		int handle_mouse_down(const SDL_Event& ev, city** c);
		void update_view();
		int check_line_drawing(int x, int y);
		int try_choose_with_mouse(city** c);
		int draw_line_by_sq(const coord& c1, const coord& c2, int r, int g, int b);
		bool unit_not_at(int x, int y, const unit* u) const;
		void handle_action_mouse_down(const SDL_Event& ev);
		action handle_action_mouse_up(const SDL_Event& ev);
		void check_unit_movement_orders();
		int try_perform_action(const action& a, city** c);
		void update_tile_info(int x, int y);
		void display_tile_info() const;
		std::string unit_strength_info_string(const unit* u) const;
		bool write_unit_info(const unit* u, int* written_lines) const;
		void add_gui_msg(const std::string& s);
		void update_action_buttons();
		void add_revolution_confirm_window(const char* msg);
		int start_revolution(const widget_window* w);
		void add_choose_revolution_window();
		int choose_government(unsigned int gov_id, const widget_window* w);
		void check_revolution_notifier(unsigned int adv_id);
		int unit_wait();
		int unit_center();
		int unit_skip();
		int unit_fortify();
		int unit_found_city();
		int unit_improve(improvement_type i);
		int unit_load();
		int unit_unload();
		void draw_overlays();
		void draw_action_buttons();
		void clear_action_buttons();
		rect action_button_dim(int num) const;
		std::map<unsigned int, unit*>::const_iterator current_unit;
		std::map<unsigned int, std::list<coord> > unit_movement_orders;
		bool blink_unit;
		int timer;
		civilization* myciv;
		std::list<coord> path_to_draw;
		ai* internal_ai;
		coord sidebar_info_display;
		std::list<std::string> gui_msg_queue;
		std::list<button*> action_buttons;
		action action_button_action;
		std::string ruleset_name;
		static const color popup_text_color;
		static const color popup_button_color;
		static const color popup_background_color;
};

#endif
