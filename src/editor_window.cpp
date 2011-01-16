#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "editor_window.h"
#include "serialize.h"

editor_window::editor_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_)
	: main_window(screen_, data_, res_, 4),
	chosen_terrain(2),
	brush_size(1),
	sidebar_terrain_xstart(10),
	sidebar_terrain_ystart(80)
{
	int xpos = sidebar_terrain_xstart;
	int ypos = sidebar_terrain_ystart;
	for(int i = 0; !data.m.resconf.resource_name[i].empty(); i++) {
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
	xpos = sidebar_terrain_xstart;
	ypos = sidebar_terrain_ystart + tile_h * (1 + (num_terrain_types + 2) / 3);
	for(int i = 1; i <= 5; i += 2) {
		rect button_dim(xpos, ypos, tile_w, tile_h);
		char buf[256];
		snprintf(buf, 255, "%dx%d", i, i);
		sidebar_buttons.push_back(new plain_button(button_dim,
					buf, &res.font, color(255, 255, 255),
					color(0, 0, 0),
					boost::bind(&editor_window::on_size_button, this, i)));
		xpos += tile_w;
	}
}

editor_window::~editor_window()
{
	while(!sidebar_buttons.empty()) {
		delete sidebar_buttons.back();
		sidebar_buttons.pop_back();
	}
}

int editor_window::on_terrain_button(int val)
{
	chosen_terrain = val;
	return 0;
}

int editor_window::on_size_button(int val)
{
	brush_size = val;
	return 0;
}

void editor_window::draw_sidebar()
{
	draw_minimap();
	std::for_each(sidebar_buttons.begin(),
			sidebar_buttons.end(),
			std::bind2nd(std::mem_fun(&button::draw), screen));

	// chosen terrain
	draw_rect(sidebar_terrain_xstart + (tile_w * (chosen_terrain % 3)),
			sidebar_terrain_ystart + (tile_h * (chosen_terrain / 3)),
			tile_w, tile_h, color(255, 255, 255), 1, screen);

	// chosen size
	draw_rect(sidebar_terrain_xstart + tile_w * (brush_size / 2),
			sidebar_terrain_ystart + (tile_h * (1 + (num_terrain_types + 2) / 3)),
			tile_w, tile_h, color(255, 255, 255), 1, screen);
}

int editor_window::handle_window_input(const SDL_Event& ev)
{
	main_window::handle_window_input(ev);
	return handle_input_gui_mod(ev);
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
					w->add_button(10, 76, "OK", boost::bind(&editor_window::on_new_map,
								this, boost::lambda::_1));
					w->add_button(110, 76, "Cancel", widget_close);
					w->add_key_handler(SDLK_ESCAPE, widget_close);
					add_subwindow(w);
				}
				if(k == SDLK_q || k == SDLK_ESCAPE) {
					return 1;
				}
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
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

int editor_window::on_new_map(const widget_window* w)
{
	int xv = 0;
	int yv = 0;
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
	if(xv >= 40 && yv >= 40) {
		// new map
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
		modify_map(mouse_down_sqx, mouse_down_sqy);
	}
	else {
		return check_button_click(sidebar_buttons, ev);
	}
	return 0;
}

void editor_window::modify_map(int x, int y)
{
	for(int i = -(brush_size / 2); i <= brush_size / 2; i++) {
		for(int j = -(brush_size / 2); j <= brush_size / 2; j++) {
			data.m.set_data(x + i, y + j, chosen_terrain);
			if(!data.m.resconf.is_water_tile(chosen_terrain)) {
				for(int k = -1; k <= 1; k++) {
					for(int l = -1; l <= 1; l++) {
						int neighbour = data.m.get_data(x + i + k, y + j + l);
						if(data.m.resconf.is_ocean_tile(neighbour)) {
							data.m.set_data(x + i + k, y + j + l, 
									data.m.resconf.get_sea_tile());
						}
					}
				}
			}
		}
	}
}

int editor_window::handle_mousemotion(const SDL_Event& ev)
{
	if(mouse_down_sqx >= 0) {
		modify_map(mouse_down_sqx, mouse_down_sqy);
	}
	return 0;
}

