#include <sstream>
#include <boost/bind.hpp>
#include "gui-utils.h"

widget::widget(const std::string& name_, const rect& dim_)
	: name(name_),
	dim(dim_)
{
}

const std::string& widget::get_name() const
{
	return name;
}

button::button(const std::string& name_, const rect& dim_, boost::function<int()> onclick_)
	: widget(name_, dim_),
	onclick(onclick_)
{
}

int button::handle_input(const SDL_Event& ev)
{
	if(ev.type == SDL_MOUSEBUTTONUP && 
			in_rectangle(dim, ev.button.x, ev.button.y)) {
		if(onclick) {
			return onclick();
		}
	}
	return 0;
}

int button::draw_surface(SDL_Surface* screen, SDL_Surface* surf)
{
	SDL_Rect dest;
	dest.x = dim.x;
	dest.y = dim.y;

	if(SDL_BlitSurface(surf, NULL, screen, &dest)) {
		fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

texture_button::texture_button(const std::string& name_, const rect& dim_, SDL_Surface* surf_, 
		boost::function<int()> onclick_)
	: button(name_, dim_, onclick_),
	surf(surf_)
{
}

texture_button::~texture_button()
{
}

int texture_button::draw(SDL_Surface* screen)
{
	return draw_surface(screen, surf);
}

plain_button::plain_button(const rect& dim_, const char* text, const TTF_Font* font,
		const color& bg_col, const color& text_col, 
		boost::function<int()> onclick_)
	: button(std::string(text), dim_, onclick_),
	surf(make_label(text, font, dim_.w, dim_.h, bg_col, text_col))
{
}

plain_button::~plain_button()
{
	SDL_FreeSurface(surf);
}

int plain_button::draw(SDL_Surface* screen)
{
	return draw_surface(screen, surf);
}

SDL_Surface* make_label(const char* text, const TTF_Font* font, 
		int w, int h, const color& bg_col, const color& text_col)
{
	SDL_Surface* button_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, w,
			h, 32, rmask, gmask, bmask, 0);
	if(!button_surf)
		return NULL;
	Uint32 button_bg_col = SDL_MapRGB(button_surf->format, bg_col.r, bg_col.g, bg_col.b);
	SDL_FillRect(button_surf, NULL, button_bg_col);
	const int fade_steps = 10;
	for(int i = 0; i < fade_steps; i++) {
		color fade_col(bg_col.r * i / fade_steps,
				bg_col.g * i / fade_steps,
				bg_col.b * i / fade_steps);
		draw_line(button_surf, i, i, w - i, i, fade_col);
		draw_line(button_surf, i, i, i, h - i, fade_col);
		draw_line(button_surf, w - i, i, w - i, h - i, fade_col);
		draw_line(button_surf, i, h - i, w - i, h - i, fade_col);
	}
	SDL_Color text_sdl_col = {text_col.r, text_col.g, text_col.b};
	SDL_Surface* button_text = TTF_RenderUTF8_Blended((TTF_Font*)font, text, text_sdl_col);
	if(button_text) {
		SDL_Rect pos;
		pos.x = w / 2 - button_text->w / 2;
		pos.y = h / 2 - button_text->h / 2;
		if(SDL_BlitSurface(button_text, NULL, button_surf, &pos)) {
			fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
			SDL_FreeSurface(button_surf);
			button_surf = NULL;
		}
		SDL_FreeSurface(button_text);
	}
	else {
		SDL_FreeSurface(button_surf);
		button_surf = NULL;
	}
	return button_surf;
}

int draw_rect(int x, int y, int w, int h, const color& c, 
		int border_width, SDL_Surface* screen)
{
	Uint32 sdl_col = SDL_MapRGB(screen->format, c.r, c.g, c.b);
	if(border_width == 0) {
		SDL_Rect dest;
		dest.x = x;
		dest.y = y;
		dest.w = w;
		dest.h = h;
		return SDL_FillRect(screen, &dest, sdl_col);
	}
	else {
		SDL_Rect dest;
		dest.x = x;
		dest.y = y;
		dest.w = w;
		dest.h = border_width;
		if(SDL_FillRect(screen, &dest, sdl_col))
			return -1;
		dest.w = border_width;
		dest.h = h;
		if(SDL_FillRect(screen, &dest, sdl_col))
			return -1;
		dest.x = x + w - border_width;
		dest.y = y;
		if(SDL_FillRect(screen, &dest, sdl_col))
			return -1;
		dest.x = x;
		dest.y = y + h - border_width;
		dest.w = w;
		dest.h = border_width;
		return SDL_FillRect(screen, &dest, sdl_col);
	}
}

int draw_image(int x, int y, const SDL_Surface* tile, SDL_Surface* screen)
{
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;

	if(SDL_BlitSurface((SDL_Surface*)tile, NULL, screen, &dest)) {
		fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

int draw_road_overlays(const map& m, 
		int x, int y, int xpos, int ypos, 
		const tileset& terrains,
		SDL_Surface* screen)
{
	SDL_Rect dest;
	dest.x = xpos;
	dest.y = ypos;
	for(int i = 0; i < 3; i++) {
		for(int j = 0; j < 3; j++) {
			if((m.get_improvements_on(x + i - 1, y + j - 1) & improv_road) &&
				terrains.road_overlays[3 * i + j] != NULL) {
				if(SDL_BlitSurface(terrains.road_overlays[3 * i + j], NULL, screen, &dest)) {
					fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
					return 1;
				}
			}
		}
	}
	return 0;
}

#ifdef BLEND_TERRAIN
color blend_colors(const color& c1, const color& c2, float bl)
{
	color blendcol;
	blendcol.r = c2.r + ((c1.r - c2.r) * bl);
	blendcol.g = c2.g + ((c1.g - c2.g) * bl);
	blendcol.b = c2.b + ((c1.b - c2.b) * bl);
	return blendcol;
}

void blend_terrain_colors(int x, int y, int xpos, int ypos,
		const map& m,
		const tileset& terrains,
		SDL_Surface* screen)
{
	int val = m.get_data(x, y);
	const int num_steps = BLEND_TERRAIN;
	float blend_ratio[num_steps];
	for(int j = 0; j < num_steps; j++)
		blend_ratio[j] = (num_steps + j + 1) / (2.0f * num_steps + 1.0f);
	int lval = m.get_data(x - 1, y);
	if(lval >= 0 && lval < (int)terrains.textures.size()) {
		for(int j = 0; j < num_steps; j++) {
			for(int i = 0; i < terrains.tile_h; i++) {
				color col1 = sdl_get_pixel(terrains.textures[val],
						j, i);
				color col2 = sdl_get_pixel(terrains.textures[lval],
						terrains.tile_w - 1 - j, i);
				color blendcol = blend_colors(col1, col2, blend_ratio[j]);
				sdl_put_pixel(screen, xpos + j, ypos + i, blendcol);
			}
		}
	}
	int rval = m.get_data(x + 1, y);
	if(rval >= 0 && rval < (int)terrains.textures.size()) {
		for(int j = 0; j < num_steps; j++) {
			for(int i = 0; i < terrains.tile_h; i++) {
				color col1 = sdl_get_pixel(terrains.textures[val],
						terrains.tile_w - j - 1, i);
				color col2 = sdl_get_pixel(terrains.textures[rval],
						j, i);
				color blendcol = blend_colors(col1, col2, blend_ratio[j]);
				sdl_put_pixel(screen, xpos + terrains.tile_w - 1 - j,
						ypos + i, blendcol);
			}
		}
	}
	int uval = m.get_data(x, y - 1);
	if(uval >= 0 && uval < (int)terrains.textures.size()) {
		for(int j = 0; j < num_steps; j++) {
			for(int i = 0; i < terrains.tile_w; i++) {
				color col1 = sdl_get_pixel(terrains.textures[val],
						i, j);
				color col2 = sdl_get_pixel(terrains.textures[uval],
						i, terrains.tile_h - 1 - j);
				color blendcol = blend_colors(col1, col2, blend_ratio[j]);
				sdl_put_pixel(screen, xpos + i,
						ypos + j, blendcol);
			}
		}
	}
	int dval = m.get_data(x, y + 1);
	if(dval >= 0 && dval < (int)terrains.textures.size()) {
		for(int j = 0; j < num_steps; j++) {
			for(int i = 0; i < terrains.tile_w; i++) {
				color col1 = sdl_get_pixel(terrains.textures[val],
						i, terrains.tile_h - 1 - j);
				color col2 = sdl_get_pixel(terrains.textures[dval],
						i, j);
				color blendcol = blend_colors(col1, col2, blend_ratio[j]);
				sdl_put_pixel(screen, xpos + i,
						ypos + terrains.tile_h - 1 - j, blendcol);
			}
		}
	}
}
#endif

int draw_rivers(int x, int y, const map& m, const tileset& terrains,
		const SDL_Rect& dest, SDL_Surface* screen)
{
	if(m.has_river(x, y)) {
		for(int n = 0; n < 4; n++) {
			int i = n == 1 ? -1 : n == 2 ? 1 : 0;
			int j = n == 0 ? -1 : n == 3 ? 1 : 0;
			if(m.has_river(x + i, y + j) ||
					m.resconf.is_water_tile(m.get_data(x + i, y + j))) {
				SDL_Surface* s = terrains.river_overlays[n];
				if(s) {
					if(SDL_BlitSurface(s, NULL, screen, (SDL_Rect*)&dest)) {
						fprintf(stderr, "Unable to blit river surface: %s\n", SDL_GetError());
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

int draw_terrain_tile(int x, int y, int xpos, int ypos, bool shade,
		const map& m, 
		const tileset& terrains,
		const std::map<unsigned int, SDL_Surface*>& resource_images,
		bool draw_improvements,
		bool draw_resources,
		const std::set<unsigned int>* researched_advances,
		SDL_Surface* screen)
{
	SDL_Rect dest;
	dest.x = xpos;
	dest.y = ypos;
	int val = m.get_data(x, y);
	if(val < 0 || val >= (int)terrains.textures.size()) {
		fprintf(stderr, "Terrain at %d not loaded at (%d, %d)\n", val,
				x, y);
		return 1;
	}
	if(SDL_BlitSurface(terrains.textures[val], NULL, screen, &dest)) {
		fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
		return 1;
	}
	if(draw_rivers(x, y, m, terrains, dest, screen)) {
		return 1;
	}

#ifdef BLEND_TERRAIN
	blend_terrain_colors(x, y, xpos, ypos, m, terrains, screen);
#endif

	if(draw_improvements) {
		int im = m.get_improvements_on(x, y);
		SDL_Surface* imp_surf = NULL;
		if(im & improv_irrigation)
			imp_surf = terrains.irrigation_overlay;
		else if(im & improv_mine)
			imp_surf = terrains.mine_overlay;
		if(imp_surf) {
			if(SDL_BlitSurface(imp_surf, NULL, screen, &dest)) {
				fprintf(stderr, "Unable to blit surface: %s\n", SDL_GetError());
				return 1;
			}
		}
		if(im & improv_road) {
			if(draw_road_overlays(m, x, y, xpos, ypos, terrains, screen))
				return 1;
		}
	}

	if(draw_resources) {
		int res = m.get_resource(x, y);
		if(res) {
			std::map<unsigned int, SDL_Surface*>::const_iterator it =
				resource_images.find(res);
			if(it != resource_images.end()) {
				bool draw = false;
				if(researched_advances == NULL) {
					draw = true;
				}
				else {
					resource_map::const_iterator it = m.rmap.find(res);
					if(it != m.rmap.end()) {
						if(it->second.needed_advance == 0 ||
						researched_advances->find(it->second.needed_advance) !=
						researched_advances->end()) {
							draw = true;
						}
					}
				}
				if(draw) {
					if(SDL_BlitSurface(it->second, NULL, screen, &dest)) {
						fprintf(stderr, "Unable to blit resource surface: %s\n", SDL_GetError());
						return 1;
					}
				}
			}
		}
	}

	if(shade) {
		color c(0, 0, 0);
		for(int i = dest.y; i < dest.y + terrains.tile_h; i++) {
			for(int j = dest.x; j < dest.x + terrains.tile_w; j++) {
				if((i % 2) == (j % 2)) {
					sdl_put_pixel(screen, j, i, c);
				}
			}
		}
	}
	return 0;
}

gui_resources::gui_resources(const TTF_Font& f, int tile_w, int tile_h, 
		SDL_Surface* food_, SDL_Surface* prod_, SDL_Surface* comm_)
	: terrains(tile_w, tile_h),
       	font(f),
	food_icon(food_),
	prod_icon(prod_),
	comm_icon(comm_)
{
}

gui_resources::~gui_resources()
{
	for(UnitImageMap::iterator it = unit_images.begin();
			it != unit_images.end();
			++it) {
		SDL_FreeSurface(it->second);
	}
}

gui_data::gui_data(map& mm, pompelmous& rr)
	: m(mm),
	r(rr)
{
}

tileset::tileset(int w, int h)
	: tile_w(w),
	tile_h(h)
{
}

SDL_Surface* gui_resources::get_unit_tile(const unit& u, const color& c)
{
	if(u.uconf_id < 0 || u.uconf_id >= (int)plain_unit_images.size()) {
		fprintf(stderr, "Image for Unit ID %d not loaded\n", u.uconf_id);
		return NULL;
	}
	SDL_Surface* surf;
	UnitImageMap::const_iterator it = unit_images.find(std::make_pair(u.uconf_id,
			       c));
	if(it == unit_images.end()) {
		// load image
		SDL_Surface* plain = plain_unit_images[u.uconf_id];
		if(!plain) {
			fprintf(stderr, "%s: could not load image file for unit '%s'.\n",
					__func__, u.uconf->unit_name.c_str());
			return NULL;
		}
		SDL_Surface* result = SDL_DisplayFormatAlpha(plain);
		sdl_change_pixel_color(result, color(0, 255, 255), c);
		unit_images.insert(std::make_pair(std::make_pair(u.uconf_id, c),
					result));
		surf = result;
	}
	else {
		surf = it->second;
	}
	return surf;
}

int check_button_click(const std::list<button*>& buttons,
		const SDL_Event& ev)
{
	if(ev.type != SDL_MOUSEBUTTONDOWN && ev.type != SDL_MOUSEBUTTONUP)
		return 0;
	for(std::list<button*>::const_iterator it = buttons.begin();
			it != buttons.end();
			++it) {
		if(in_rectangle((*it)->dim, ev.button.x, ev.button.y)) {
			if((*it)->onclick) {
				return (*it)->onclick();
			}
			else
				return 0;
		}
	}
	return 0;
}

window::window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_)
	: screen(screen_),
	data(data_),
	res(res_)
{
}

void window::add_subwindow(window* w)
{
	subwindows.push_back(w);
}

int window::draw()
{
	if(subwindows.empty()) {
		return draw_window();
	}
	else {
		(*subwindows.begin())->draw();
		return 0;
	}
}

int window::handle_input(const SDL_Event& ev)
{
	if(subwindows.empty()) {
		return handle_window_input(ev);
	}
	else {
		std::list<window*>::iterator it = subwindows.begin();
		int p = (*it)->handle_input(ev);
		if(p) {
			delete *it;
			subwindows.erase(it);
		}
		return 0;
	}
}

int window::process(int ms)
{
	int ret = process_window(ms);
	if(ret)
		return ret;
	std::for_each(subwindows.begin(), subwindows.end(), 
			std::bind2nd(std::mem_fun(&window::process), ms));
	return 0;
}

void window::init_turn()
{
	init_window_turn();
	std::for_each(subwindows.begin(), subwindows.end(), std::mem_fun(&window::init_turn));
}

int window::num_subwindows() const
{
	return subwindows.size();
}

input_text_window::input_text_window(SDL_Surface* screen_, gui_data& data_,
		gui_resources& res_,
		const rect& rect_,
		const std::string& info_string_,
		const std::string& default_string,
		const color& bg_color_,
		const color& button_color,
		const color& button_text_color_,
		boost::function<int(const std::string&)> on_ok_,
		boost::function<int(const std::string&)> on_cancel_)
	: window(screen_, data_, res_),
	dims(rect_),
	info_string(info_string_),
	bg_color(bg_color_),
	text_color(button_text_color_),
	on_ok_func(on_ok_),
	on_cancel_func(on_cancel_),
	tb(&res_.font, rect(dims.x + 4, dims.y + dims.h - 40,
				dims.w - 8, 16),
			button_color, text_color,
			std::string("textbox"), default_string)
{
	buttons.push_back(new plain_button(rect(dims.x + dims.w - 100,
					dims.y + dims.h - 20,
					46, 16), "OK", &res.font,
					button_color, text_color,
					boost::bind(&input_text_window::on_ok, this)));
	buttons.push_back(new plain_button(rect(dims.x + dims.w - 50,
					dims.y + dims.h - 20,
					46, 16), "Cancel", &res.font,
					button_color, text_color,
					boost::bind(&input_text_window::on_cancel, this)));
}

int input_text_window::on_ok()
{
	return on_ok_func(tb.get_text());
}

int input_text_window::on_cancel()
{
	return on_cancel_func(tb.get_text());
}

int input_text_window::handle_window_input(const SDL_Event& ev)
{
	if(ev.type == SDL_MOUSEBUTTONDOWN) {
		return check_button_click(buttons, ev);
	}
	else if(ev.type == SDL_KEYDOWN) {
		if(ev.key.keysym.sym == SDLK_ESCAPE) {
			return on_cancel_func(tb.get_text());
		}
		else if(ev.key.keysym.sym == SDLK_KP_ENTER ||
				ev.key.keysym.sym == SDLK_RETURN) {
			return on_ok_func(tb.get_text());
		}
	}
	tb.handle_input(ev);
	return 0;
}

int input_text_window::draw_window()
{
	draw_plain_rectangle(screen, dims.x, dims.y, dims.w, dims.h, bg_color);
	if(!info_string.empty())
		draw_text(screen, &res.font, info_string.c_str(), dims.x + 6,
				dims.y + 6, text_color.r, text_color.g, text_color.b, false);
	tb.draw(screen);
	std::for_each(buttons.begin(),
			buttons.end(),
			std::bind2nd(std::mem_fun(&button::draw), screen));
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
	}
	return 0;
}

int empty_click_handler(const std::string& s)
{
	return 1;
}

textbox::textbox(const TTF_Font* font_, const rect& dim_,
		const color& bg_color_, const color& text_color_,
		const std::string& name_, const std::string& default_string_)
	: widget(name_, dim_),
	text(default_string_),
	font(font_),
	bg_color(bg_color_),
	text_color(text_color_)
{
}

int textbox::draw(SDL_Surface* screen)
{
	draw_plain_rectangle(screen, dim.x, dim.y, dim.w, dim.h, bg_color);
	if(!text.empty())
		draw_text(screen, font, text.c_str(), dim.x + 2,
				dim.y + 2, text_color.r, text_color.g, text_color.b, false);
	return 0;
}

int textbox::handle_input(const SDL_Event& ev)
{
	if(ev.type == SDL_KEYDOWN) {
		if(ev.key.keysym.unicode >= 32 && ev.key.keysym.unicode < 127) {
			// only ASCII for now
			text += (char)ev.key.keysym.unicode;
		}
		else if(ev.key.keysym.unicode == 8 && !text.empty()) {
			// backspace
			text.resize(text.size() - 1);
		}
	}
	return 0;
}

const std::string& textbox::get_text() const
{
	return text;
}

numeric_textbox::numeric_textbox(const TTF_Font* font_, const rect& dim_,
		const color& bg_color_, const color& text_color_,
		const std::string& name_,
		int default_num)
	: textbox(font_, dim_, bg_color_, text_color_, name_, std::string(""))
{
	std::stringstream s;
	s << default_num;
	text = s.str();
}

int numeric_textbox::handle_input(const SDL_Event& ev)
{
	if(ev.type == SDL_KEYDOWN) {
		if(ev.key.keysym.unicode >= '0' && ev.key.keysym.unicode <= '9') {
			text += (char)ev.key.keysym.unicode;
		}
		else if(ev.key.keysym.unicode == 8 && !text.empty()) {
			// backspace
			text.resize(text.size() - 1);
		}
	}
	return 0;
}

int numeric_textbox::get_numeric_value() const
{
	int res = 0;
	std::stringstream(text) >> res;
	return res;
}

checkbox::checkbox(const rect& dim_,
		const color& box_col, const color& frame_col_,
		const std::string& name_, bool default_checked)
	: button(name_, dim_, boost::bind(&checkbox::on_click, this)),
	  checked(default_checked),
	  frame_col(frame_col_)
{
	surf = SDL_CreateRGBSurface(SDL_SWSURFACE, dim.w,
			dim.h, 32, rmask, gmask, bmask, 0);
	if(!surf)
		return;
	Uint32 bg_col = SDL_MapRGB(surf->format, box_col.r, box_col.g, box_col.b);
	SDL_FillRect(surf, NULL, bg_col);
	draw_rect(0, 0, dim.w, dim.h, frame_col, 1, surf);
}

checkbox::~checkbox()
{
	if(surf)
		SDL_FreeSurface(surf);
}

int checkbox::draw(SDL_Surface* screen)
{
	draw_surface(screen, surf);
	if(checked) {
		draw_line(screen, dim.x, dim.y, dim.x + dim.w, dim.y + dim.h,
				frame_col);
		draw_line(screen, dim.x + dim.w, dim.y, dim.x, dim.y + dim.h,
				frame_col);
	}
	return 0;
}

int checkbox::on_click()
{
	checked = !checked;
	return 0;
}

bool checkbox::get_checked() const
{
	return checked;
}

widget_window::widget_window(SDL_Surface* screen_, gui_data& data_,
		gui_resources& res_,
		const rect& rect_,
		const color& bg_color_)
	: window(screen_, data_, res_),
	dim(rect_),
	bg_color(bg_color_),
	text_color(color(0, 0, 0)),
	button_color(255, 255, 255),
	focus_widget(NULL)
{
}

widget_window::~widget_window()
{
	while(!buttons.empty()) {
		delete buttons.back();
		buttons.pop_back();
	}
	while(!numeric_textboxes.empty()) {
		delete numeric_textboxes.back();
		numeric_textboxes.pop_back();
	}
}

void widget_window::set_focus_widget(int x, int y)
{
	focus_widget = NULL;
	for(std::list<numeric_textbox*>::iterator it = numeric_textboxes.begin();
			it != numeric_textboxes.end();
			++it) {
		if(in_rectangle((*it)->dim, x, y)) {
			focus_widget = *it;
			return;
		}
	}
	for(std::list<button*>::iterator it = buttons.begin();
			it != buttons.end();
			++it) {
		if(in_rectangle((*it)->dim, x, y)) {
			if((*it)->onclick)
				focus_widget = *it;
			return;
		}
	}
}

int widget_window::handle_window_input(const SDL_Event& ev)
{
	if(ev.type == SDL_MOUSEBUTTONDOWN) {
		set_focus_widget(ev.button.x, ev.button.y);
	}
	if(ev.type == SDL_KEYDOWN) {
		std::map<SDLKey, boost::function<int(const widget_window*)> >::const_iterator it = key_handlers.find(ev.key.keysym.sym);
		if(it != key_handlers.end()) {
			int ret;
			ret = it->second(this);
			if(ret)
				return ret;
		}
	}
	for(std::list<numeric_textbox*>::iterator it = numeric_textboxes.begin();
			it != numeric_textboxes.end();
			++it) {
		if(focus_widget != *it)
			continue;
		int ret;
		if((ret = (*it)->handle_input(ev)) != 0)
			return ret;
	}
	for(std::list<button*>::iterator it = buttons.begin();
			it != buttons.end();
			++it) {
		if(focus_widget != *it)
			continue;
		int ret;
		if((ret = (*it)->handle_input(ev)) != 0)
			return ret;
	}
	return 0;
}

int widget_window::draw_window()
{
	draw_plain_rectangle(screen, dim.x, dim.y, dim.w, dim.h, bg_color);
	std::for_each(numeric_textboxes.begin(), numeric_textboxes.end(), std::bind2nd(std::mem_fun(&widget::draw), screen));
	std::for_each(buttons.begin(), buttons.end(), std::bind2nd(std::mem_fun(&widget::draw), screen));
	if(focus_widget) {
		draw_rect(focus_widget->dim.x, focus_widget->dim.y,
				focus_widget->dim.w, focus_widget->dim.h,
				color(255, 255, 255), 1, screen);
	}
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
		return 1;
	}
	else {
		return 0;
	}
}

void widget_window::set_text_color(const color& c)
{
	text_color = c;
}

void widget_window::set_button_color(const color& c)
{
	button_color = c;
}

void widget_window::add_label(int x, int y, int w, int h, const std::string& text)
{
	buttons.push_back(new plain_button(rect(dim.x + x, dim.y + y, w, h), text.c_str(),
				&res.font, button_color, text_color,
				NULL));
}

void widget_window::add_numeric_textbox(int x, int y, const std::string& text, int val)
{
	numeric_textboxes.push_back(new numeric_textbox(&res.font,
				rect(dim.x + x, dim.y + y, 80, 16), button_color, text_color,
					text, val));
}

void widget_window::add_checkbox(int x, int y, int w, int h, const std::string& text, bool checked)
{
	checkboxes.push_back(new checkbox(rect(dim.x + x, dim.y + y, w, h), button_color,
			       text_color, text, checked));
}

void widget_window::add_button(int x, int y, int w, int h, const std::string& text, boost::function<int(const widget_window*)> cb)
{
	buttons.push_back(new plain_button(rect(dim.x + x, dim.y + y, w, h), text.c_str(),
				&res.font, button_color, text_color,
				boost::bind(&widget_window::on_button_click, this, cb)));
}

void widget_window::add_key_handler(SDLKey k, boost::function<int(const widget_window*)> cb)
{
	key_handlers.insert(std::make_pair(k, cb));
}

int widget_window::on_button_click(boost::function<int(const widget_window*)> cb)
{
	return cb(this);
}

int widget_close(const widget_window* w)
{
	return 1;
}

