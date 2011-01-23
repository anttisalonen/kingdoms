#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "editor_window.h"
#include "serialize.h"
#include "filesystem.h"

editor_window::editor_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_)
	: main_window(screen_, data_, res_, 4),
	current_tool(editor_tool_terrain),
	current_tool_index(2),
	brush_size(1),
	sidebar_terrain_xstart(10),
	sidebar_terrain_ystart(80),
	sidebar_startpos_button_width(100),
	sidebar_coastal_protection_button_width(100),
	quitting(false),
	coastal_protection(false)
{
	// terrain buttons
	int xpos = sidebar_terrain_xstart;
	int ypos = sidebar_terrain_ystart;
	int i;
	for(i = 0; !data.m.resconf.resource_name[i].empty(); i++) {
		rect button_dim(xpos, ypos, tile_w, tile_h);
		sidebar_buttons.push_back(new texture_button(std::string(""), button_dim,
						res.terrains.textures[i],
						boost::bind(&editor_window::on_terrain_button, this, i)));
		if((i % 3) == 2) {
			ypos += tile_h;
			xpos = sidebar_terrain_xstart;
		}
		else {
			xpos += tile_w;
		}
	}

	// resource buttons
	xpos = sidebar_terrain_xstart;
	ypos += tile_h * 2;
	i = 0;
	sidebar_resource_ystart = ypos;
	for(resource_map::const_iterator it = data.m.rmap.begin();
			it != data.m.rmap.end();
			++it, i++) {
		rect button_dim(xpos, ypos, tile_w, tile_h);
		std::map<unsigned int, SDL_Surface*>::const_iterator rit = res.resource_images.find(it->first);
		if(rit != res.resource_images.end()) {
			sidebar_buttons.push_back(new texture_button(std::string(""), button_dim,
						rit->second,
						boost::bind(&editor_window::on_resource_button, this, it->first)));
			if((i % 3) == 2) {
				ypos += tile_h;
				xpos = sidebar_terrain_xstart;
			}
			else {
				xpos += tile_w;
			}
		}
	}

	// river
	xpos = sidebar_terrain_xstart;
	ypos += tile_h * 2;
	sidebar_river_ystart = ypos;
	rect river_dim(xpos, ypos, tile_w, tile_h);
	sidebar_buttons.push_back(new texture_button(std::string(""), river_dim,
				res.terrains.river_overlays[0],
				boost::bind(&editor_window::on_river_button, this)));

	// brush size buttons
	xpos = sidebar_terrain_xstart;
	ypos += tile_h * 2;
	sidebar_brush_size_ystart = ypos;
	for(i = 1; i <= 5; i += 2) {
		rect button_dim(xpos, ypos, tile_w, tile_h);
		char buf[256];
		snprintf(buf, 255, "%dx%d", i, i);
		sidebar_buttons.push_back(new plain_button(button_dim,
					buf, &res.font, color(255, 255, 255),
					color(0, 0, 0),
					boost::bind(&editor_window::on_size_button, this, i)));
		xpos += tile_w;
	}

	xpos = sidebar_terrain_xstart;
	ypos += tile_h * 2;
	sidebar_startpos_ystart = ypos;
	if(data.r.civs.size() > 0) {
		sidebar_buttons.push_back(new plain_button(rect(xpos, ypos, sidebar_startpos_button_width, tile_h),
					"Start positions", &res.font, color(255, 255, 255),
					color(0, 0, 0),
					boost::bind(&editor_window::on_startposition_button, this)));
	}

	xpos = sidebar_terrain_xstart;
	ypos += tile_h * 2;
	sidebar_coastal_protection_ystart = ypos;
	sidebar_buttons.push_back(new plain_button(rect(xpos, ypos, sidebar_coastal_protection_button_width, tile_h),
				"Protect coasts", &res.font, color(255, 255, 255),
				color(0, 0, 0),
				boost::bind(&editor_window::on_coastal_protection_button, this)));
}

editor_window::~editor_window()
{
	while(!sidebar_buttons.empty()) {
		delete sidebar_buttons.back();
		sidebar_buttons.pop_back();
	}
}

int editor_window::on_resource_button(unsigned int res_id)
{
	current_tool = editor_tool_resource;
	current_tool_index = res_id;
	return 0;
}

int editor_window::on_terrain_button(int val)
{
	current_tool = editor_tool_terrain;
	current_tool_index = val;
	return 0;
}

int editor_window::on_size_button(int val)
{
	brush_size = val;
	return 0;
}

int editor_window::on_river_button()
{
	current_tool = editor_tool_river;
	current_tool_index = 0;
	return 0;
}

bool editor_window::draw_starting_positions()
{
	return true;
}

int editor_window::on_coastal_protection_button()
{
	coastal_protection = !coastal_protection;
	return 0;
}

widget_window* editor_window::standard_popup(int win_width, int win_height) const
{
	color text_color(255, 255, 255);
	color button_color(80, 0, 0);
	widget_window* w = new widget_window(screen, data, res,
			rect(screen->w / 2 - 100, 100,
				win_width, win_height),
			color(160, 0, 0));
	w->set_text_color(text_color);
	w->set_button_color(button_color);
	w->add_key_handler(SDLK_ESCAPE, widget_close);
	return w;
}

int editor_window::on_startposition_button()
{
	int win_height = screen->h - 200;
	int win_width = 200;
	widget_window* w = standard_popup(win_width, win_height);
	w->add_label(10, 2, win_width - 20, 16, "Starting position");
	w->add_button(20, win_height - 24, win_width - 40, 16, "Cancel", widget_close);
	int yp = 30;
	for(unsigned int i = 0; i < data.r.civs.size(); i++) {
		w->add_button(2, yp, win_width - 4, 16, data.r.civs[i]->civname.c_str(),
				boost::bind(&editor_window::on_civ_startpos,
					this, i, boost::lambda::_1));
		yp += 24;
		if(yp >= win_height - 80)
			break;
	}
	add_subwindow(w);
	return 0;
}

int editor_window::on_civ_startpos(int civid, const widget_window* w)
{
	current_tool = editor_tool_startpos;
	current_tool_index = civid;
	return 1;
}

void editor_window::draw_sidebar()
{
	draw_minimap();
	std::for_each(sidebar_buttons.begin(),
			sidebar_buttons.end(),
			std::bind2nd(std::mem_fun(&button::draw), screen));

	if(current_tool == editor_tool_terrain) {
		draw_rect(sidebar_terrain_xstart + (tile_w * (current_tool_index % 3)),
				sidebar_terrain_ystart + (tile_h * (current_tool_index / 3)),
				tile_w, tile_h, color(255, 255, 255), 1, screen);
	}

	if(current_tool == editor_tool_resource) {
		draw_rect(sidebar_terrain_xstart + (tile_w * ((current_tool_index - 1) % 3)),
				sidebar_resource_ystart + (tile_h * ((current_tool_index - 1) / 3)),
				tile_w, tile_h, color(255, 255, 255), 1, screen);
	}

	if(current_tool == editor_tool_river) {
		draw_rect(sidebar_terrain_xstart,
				sidebar_river_ystart,
				tile_w, tile_h, color(255, 255, 255), 1, screen);
	}

	// chosen size
	draw_rect(sidebar_terrain_xstart + tile_w * (brush_size / 2),
			sidebar_brush_size_ystart,
			tile_w, tile_h, color(255, 255, 255), 1, screen);

	if(current_tool == editor_tool_startpos) {
		draw_rect(sidebar_terrain_xstart,
				sidebar_startpos_ystart,
				sidebar_startpos_button_width, tile_h,
				color(255, 255, 255), 1, screen);
	}

	if(coastal_protection) {
		draw_rect(sidebar_terrain_xstart,
				sidebar_coastal_protection_ystart,
				sidebar_coastal_protection_button_width, tile_h,
				color(255, 255, 255), 1, screen);
	}
}

int editor_window::handle_window_input(const SDL_Event& ev)
{
	main_window::handle_window_input(ev);
	handle_input_gui_mod(ev);
	return quitting;
}

bool editor_window::is_quitting() const
{
	return quitting;
}

void editor_window::add_quit_confirm_subwindow()
{
	int win_height = 40;
	int win_width = 200;
	widget_window* w = standard_popup(win_width, win_height);
	w->add_label(win_width / 2 - 40, 2, 80, 16, "Quit?");
	w->add_button(2, 20, win_width / 2 - 4, 16, "No", widget_close);
	w->add_button(win_width / 2 + 2, 20, win_width / 2 - 4, 16,
			"Yes",
			boost::bind(&editor_window::confirm_quit,
				this, boost::lambda::_1));
	w->add_key_handler(SDLK_y, 
			boost::bind(&editor_window::confirm_quit,
				this, boost::lambda::_1));
	w->add_key_handler(SDLK_n, widget_close);
	add_subwindow(w);
}

int editor_window::confirm_quit(const widget_window* w)
{
	quitting = true;
	return 1;
}

void editor_window::add_load_map_subwindow()
{
	int win_height = screen->h - 200;
	int win_width = 200;
	widget_window* w = standard_popup(win_width, win_height);
	w->add_button(20, win_height - 24, win_width - 40, 16, "Cancel", widget_close);
	w->add_label(win_width / 2 - 40, 2, 80, 16, "Load map");

	std::vector<boost::filesystem::path> filenames = get_files_in_directory(path_to_saved_games(),
			MAP_FILE_EXTENSION);

	int yp = 30;
	for(std::vector<boost::filesystem::path>::const_iterator it = filenames.begin();
			it != filenames.end();
			++it) {
		std::string s(it->string());
		std::string fp(it->stem().string());
		w->add_button(20, yp, win_width - 40, 16, fp, boost::bind(&editor_window::on_load_map, this,
					s, boost::lambda::_1));
		yp += 24;
		if(yp >= win_height - 80)
			break;
	}
	add_subwindow(w);
}

int editor_window::handle_input_gui_mod(const SDL_Event& ev)
{
	switch(ev.type) {
		case SDL_KEYDOWN:
			{
				SDLKey k = ev.key.keysym.sym;
				if(k == SDLK_s && (ev.key.keysym.mod & KMOD_CTRL)) {
					add_subwindow(new input_text_window(screen,
								data, res,
								rect(screen->w / 2 - 100,
									screen->h / 2 - 40,
									200, 80),
								std::string("Please enter file name:"),
								saved_filename,
								color(160, 0, 0),
								color(80, 0, 0),
								color(255, 255, 255),
								boost::bind(&editor_window::on_save,
									this, boost::lambda::_1),
								&empty_click_handler));
				}
				if(k == SDLK_n && (ev.key.keysym.mod & KMOD_CTRL)) {
					color text_color(255, 255, 255);
					color button_color(80, 0, 0);
					widget_window* w = new widget_window(screen, data, res,
							rect(screen->w / 2 - 100,
								screen->h / 2 - 50,
								200, 100), color(160, 0, 0));
					w->set_text_color(text_color);
					w->set_button_color(button_color);
					w->add_label(2, 2, 100, 16, "New map");
					w->add_label(2, 20, 100, 16, "X dimension");
					w->add_label(2, 38, 100, 16, "Y dimension");
					w->add_numeric_textbox(110, 20, "X dimension", 80);
					w->add_numeric_textbox(110, 38, "Y dimension", 60);
					w->add_label(2, 56, 100, 16, "Wrap X");
					w->add_checkbox(110, 56, 16, 16, "Wrap X", true);
					w->add_button(10, 76, 80, 16, "OK", boost::bind(&editor_window::on_new_map,
								this, boost::lambda::_1));
					w->add_button(110, 76, 80, 16, "Cancel", widget_close);
					w->add_key_handler(SDLK_ESCAPE, widget_close);
					add_subwindow(w);
				}
				if(k == SDLK_l && (ev.key.keysym.mod & KMOD_CTRL)) {
					add_load_map_subwindow();
				}
				if(k == SDLK_q || k == SDLK_ESCAPE) {
					add_quit_confirm_subwindow();
				}
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			save_old_mousepos();
			handle_mouse_down(ev);
			break;
		case SDL_MOUSEMOTION:
			handle_mousemotion(ev);
			break;
		default:
			break;
	}
	return 0;
}

int editor_window::on_load_map(const std::string& s, const widget_window* w)
{
	load_map(s.c_str(), data.m);
	saved_filename = boost::filesystem::path(s).stem().string();
	return 1;
}

int editor_window::on_new_map(const widget_window* w)
{
	int xv = 0;
	int yv = 0;
	bool wrap_x = true;
	for(std::list<numeric_textbox*>::const_iterator it = w->numeric_textboxes.begin();
			it != w->numeric_textboxes.end();
			++it) {
		if((*it)->get_name() == std::string("X dimension")) {
			xv = (*it)->get_numeric_value();
		}
		else if((*it)->get_name() == std::string("Y dimension")) {
			yv = (*it)->get_numeric_value();
		}
	}
	for(std::list<checkbox*>::const_iterator it = w->checkboxes.begin();
			it != w->checkboxes.end();
			++it) {
		if((*it)->get_name() == std::string("Wrap X")) {
			wrap_x = (*it)->get_checked();
		}
	}
	if(xv >= 40 && yv >= 40) {
		// new map
		data.m.set_x_wrap(wrap_x);
		data.m.resize(xv, yv);
		saved_filename = "";
		cam.cam_x = cam.cam_y = 0;
		return 1;
	}
	else {
		return 0;
	}
}

int editor_window::on_save(const std::string& s)
{
	if(s.empty())
		return 0;
	save_map(s.c_str(), data.m);
	saved_filename = s;
	return 1;
}

int editor_window::handle_mouse_down(const SDL_Event& ev)
{
	if(mouse_down_sqx >= 0) {
		switch(ev.button.button) {
			case SDL_BUTTON_LEFT:
				modify_map(mouse_down_sqx, mouse_down_sqy, false);
				break;
			case SDL_BUTTON_RIGHT:
				modify_map(mouse_down_sqx, mouse_down_sqy, true);
				break;
			default:
				break;
		}
	}
	else {
		return check_button_click(sidebar_buttons, ev);
	}
	return 0;
}

void editor_window::set_terrain(int x, int y, int terr)
{
	if(coastal_protection) {
		int t2 = data.m.get_data(x, y);
		if(data.m.resconf.is_water_tile(t2) ||
				data.m.resconf.is_water_tile(terr)) {
			return;
		}
	}
	data.m.set_data(x, y, terr);
	unsigned int res = data.m.get_resource(x, y);
	resource_map::const_iterator it = data.m.rmap.find(res);
	if(it != data.m.rmap.end()) {
		if(!it->second.allowed_on(terr))
			data.m.set_resource(x, y, 0);
	}
	if(data.m.resconf.is_water_tile(terr))
		data.m.set_river(x, y, false);
}

void editor_window::modify_map(int x, int y, bool remove)
{
	if(current_tool == editor_tool_terrain) {
		if(remove)
			return;
		for(int i = -(brush_size / 2); i <= brush_size / 2; i++) {
			for(int j = -(brush_size / 2); j <= brush_size / 2; j++) {
				set_terrain(x + i, y + j, current_tool_index);
				if(!data.m.resconf.is_water_tile(current_tool_index)) {
					for(int k = -1; k <= 1; k++) {
						for(int l = -1; l <= 1; l++) {
							int neighbour = data.m.get_data(x + i + k, y + j + l);
							if(data.m.resconf.is_ocean_tile(neighbour)) {
								set_terrain(x + i + k, y + j + l,
									data.m.resconf.get_sea_tile());
							}
						}
					}
				}
			}
		}
	}
	else if(current_tool == editor_tool_resource) {
		if(!remove) {
			resource_map::const_iterator it = data.m.rmap.find(current_tool_index);
			if(it != data.m.rmap.end()) {
				int terr = data.m.get_data(x, y);
				if(it->second.allowed_on(terr))
					data.m.set_resource(x, y, current_tool_index);
			}
		}
		else {
			if(data.m.get_resource(x, y) == (unsigned int)current_tool_index)
				data.m.set_resource(x, y, 0);
		}
	}
	else if(current_tool == editor_tool_startpos) {
		int civid = data.m.get_starter_at(x, y);
		if(!remove) {
			if(data.m.can_found_city_on(x, y)) {
				if(civid != -1) {
					data.m.remove_starting_place_of(civid);
				}
				else {
					data.m.add_starting_place(coord(x, y), current_tool_index);
				}
			}
		}
		else {
			if(civid == current_tool_index) {
				data.m.remove_starting_place_of(civid);
			}
		}
	}
	else if(current_tool == editor_tool_river) {
		if(!remove) {
			if(!data.m.resconf.is_water_tile(data.m.get_data(x, y))) {
				data.m.set_river(x, y, true);
			}
		}
		else {
			data.m.set_river(x, y, false);
		}
	}
}

int editor_window::handle_mousemotion(const SDL_Event& ev)
{
	if(mouse_down_sqx >= 0) {
		if(mouse_tile_moved() &&
				current_tool != editor_tool_startpos) {
			modify_map(mouse_down_sqx, mouse_down_sqy,
					SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(3));
		}
		save_old_mousepos();
	}
	return 0;
}

bool editor_window::mouse_tile_moved() const
{
	return old_mouse_sqx != mouse_down_sqx || old_mouse_sqy != mouse_down_sqy;
}

void editor_window::save_old_mousepos()
{
	old_mouse_sqx = mouse_down_sqx;
	old_mouse_sqy = mouse_down_sqy;
}

