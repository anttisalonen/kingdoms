#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "main_window.h"

main_window::main_window(SDL_Surface* screen_, gui_data& data_, gui_resources& res_,
		int sidebar_size_)
	: window(screen_, data_, res_),
	tile_w(32),
	tile_h(32),
	cam_total_tiles_x((screen->w + tile_w - 1) / tile_w),
	cam_total_tiles_y((screen->h + tile_h - 1) / tile_h),
	mouse_down_sqx(-1),
	mouse_down_sqy(-1),
	sidebar_size(sidebar_size_)
{
	cam.cam_x = cam.cam_y = 0;
}

main_window::~main_window()
{
}

int main_window::draw_window()
{
	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0) {
			return 1;
		}
	}
	if(sidebar_size > 0) {
		clear_sidebar();
		draw_sidebar();
	}
	clear_main_map();
	draw_main_map();
	post_draw();
	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
	if(SDL_Flip(screen)) {
		fprintf(stderr, "Unable to flip: %s\n", SDL_GetError());
		return 1;
	}
	return 0;
}

void main_window::post_draw()
{
}

void main_window::draw_sidebar()
{
}

void main_window::clear_sidebar()
{
	SDL_Rect dest;
	dest.x = 0;
	dest.y = 0;
	dest.w = sidebar_size * tile_w;
	dest.h = screen->h;
	Uint32 color = SDL_MapRGB(screen->format, 0, 0, 0);
	SDL_FillRect(screen, &dest, color);
}

int main_window::clear_main_map() const
{
	SDL_Rect dest;
	dest.x = sidebar_size * tile_w;
	dest.y = 0;
	dest.w = screen->w - sidebar_size * tile_w;
	dest.h = screen->h;
	Uint32 color = SDL_MapRGB(screen->format, 0, 0, 0);
	SDL_FillRect(screen, &dest, color);
	return 0;
}

char main_window::fog_on_tile(int x, int y) const
{
	return 2;
}

bool main_window::city_info_available(const city& c) const
{
	return true;
}

int main_window::draw_minimap() const
{
	const int minimap_w = sidebar_size * tile_w;
	const int minimap_h = sidebar_size * tile_h / 2;
	for(int i = 0; i < minimap_h; i++) {
		int y = i * data.m.size_y() / minimap_h;
		for(int j = 0; j < minimap_w; j++) {
			int x = j * data.m.size_x() / minimap_w;
			color c = get_minimap_color(x, y);
			if((c.r == 255 && c.g == 255 && c.b == 255) || 
					fog_on_tile(x, y) > 0) {
				sdl_put_pixel(screen, j, i, c);
			}
		}
	}
	SDL_UpdateRect(screen, 0, 0, minimap_w, minimap_h);

	return 0;
}

int main_window::draw_tile(const SDL_Surface* surf, int x, int y) const
{
	if(tile_visible(x, y)) {
		return draw_image(tile_xcoord_to_pixel(x),
				tile_ycoord_to_pixel(y),
				surf, screen);
	}
	return 0;
}

int main_window::draw_city(const city& c) const
{
	if(!tile_visible(c.xpos, c.ypos))
		return 0;

	if(draw_tile(res.city_images[c.civ_id], c.xpos, c.ypos))
		return 1;

	// don't draw the text on cities on gui border
	if(!tile_visible(c.xpos + 1, c.ypos) ||
	   !tile_visible(c.xpos - 1, c.ypos) ||
	   !tile_visible(c.xpos, c.ypos + 1))
		return 0;

	char buf[64];
	if(city_info_available(c)) {
		unsigned int num_turns = data.r.get_city_growth_turns(&c);
		if(num_turns == 0)
			snprintf(buf, 63, "%d %s (-)", c.get_city_size(),
					c.cityname.c_str());
		else
			snprintf(buf, 63, "%d %s (%d)", c.get_city_size(),
					c.cityname.c_str(), num_turns);
	}
	else {
		snprintf(buf, 63, "%d %s", c.get_city_size(),
				c.cityname.c_str());
	}
	buf[63] = '\0';
	if(draw_text(screen, &res.font, buf, tile_xcoord_to_pixel(c.xpos) + tile_h / 2,
			tile_ycoord_to_pixel(c.ypos) + tile_w,
			255, 255, 255, true))
		return 1;

	if(city_info_available(c)) {
		const std::string* producing = NULL;
		unsigned int num_turns_prod = 0;
		if(c.production.current_production_id >= 0) {
			if(c.production.producing_unit) {
				unit_configuration_map::const_iterator it = 
					data.r.uconfmap.find(c.production.current_production_id);
				if(it != data.r.uconfmap.end()) {
					producing = &it->second.unit_name;
					num_turns_prod = data.r.get_city_production_turns(&c,
							it->second);
				}
			}
			else {
				city_improv_map::const_iterator it = 
					data.r.cimap.find(c.production.current_production_id);
				if(it != data.r.cimap.end()) {
					producing = &it->second.improv_name;
					num_turns_prod = data.r.get_city_production_turns(&c,
							it->second);
				}
			}
		}
		if(producing) {
			if(num_turns_prod) {
				snprintf(buf, 63, "%s (%d)", producing->c_str(), 
						num_turns_prod);
			}
			else {
				snprintf(buf, 63, "%s (-)", producing->c_str());
			}
		}
		else {
			snprintf(buf, 63, "(-)");
		}
		if(draw_text(screen, &res.font, buf, tile_xcoord_to_pixel(c.xpos) + tile_h / 2,
					tile_ycoord_to_pixel(c.ypos) + 1.5 * tile_w,
					255, 255, 255, true)) {
			fprintf(stderr, "Could not draw production text.\n");
			return 1;
		}
	}
	return 0;
}

int main_window::tile_ycoord_to_pixel(int y) const
{
	return data.m.wrap_y(y - cam.cam_y) * tile_h;
}

int main_window::tile_xcoord_to_pixel(int x) const
{
	return data.m.wrap_x(x + sidebar_size - cam.cam_x) * tile_w;
}

int main_window::tile_visible(int x, int y) const
{
	bool vis_no_wrap = (in_bounds(cam.cam_x, x, cam.cam_x + cam_total_tiles_x) &&
			in_bounds(cam.cam_y, y, cam.cam_y + cam_total_tiles_y));
	if(vis_no_wrap)
		return vis_no_wrap;
	else // handle X wrapping
		return (in_bounds(cam.cam_x, x + data.m.size_x(), cam.cam_x + cam_total_tiles_x) &&
				in_bounds(cam.cam_y, y, cam.cam_y + cam_total_tiles_y));
}

bool main_window::can_draw_unit(const unit* u) const
{
	return true;
}

int main_window::draw_unit(const unit* u)
{
	if(!tile_visible(u->xpos, u->ypos)) {
		return 0;
	}
	SDL_Surface* surf = res.get_unit_tile(*u, data.r.civs[u->civ_id]->col);
	return draw_tile(surf, u->xpos, u->ypos);
}

int main_window::draw_complete_tile(int x, int y, int shx, int shy, bool terrain,
		bool resources, bool improvements, bool borders,
		boost::function<bool(const unit*)> unit_predicate,
		bool cities)
{
	char fog = fog_on_tile(x, y);
	if(fog == 0)
		return 0;
	if(terrain) {
		if(show_terrain_image(x, y, shx, shy, improvements, resources,
					true,
					fog == 1))
			return 1;
	}
	if(borders) {
		if(test_draw_border(x, y, shx, shy))
			return 1;
	}
	const std::list<unit*>& units = data.m.units_on_spot(x, y);
	std::list<unit*>::const_iterator it = units.begin();
	if(it != units.end()) {
		if((fog == 2) && unit_predicate(*it)) {
			if(draw_unit(*it))
				return 1;
		}
	}
	if(cities) {
		city* c = data.m.city_on_spot(x, y);
		if(c) {
			if(draw_city(*c))
				return 1;
		}
	}
	return 0;
}

int main_window::draw_main_map()
{
	int imax = cam.cam_y + cam_total_tiles_y;
	int jmax = cam.cam_x + cam_total_tiles_x;
	// bottom-up in y dimension for correct rendering of city names
	for(int i = imax - 1, y = cam_total_tiles_y - 1; i >= cam.cam_y; i--, y--) {
		for(int j = cam.cam_x, x = sidebar_size; j < jmax; j++, x++) {
			if(draw_complete_tile(data.m.wrap_x(j),
						data.m.wrap_y(i),
						x, y,
						true, true, true, true,
						boost::bind(&main_window::can_draw_unit,
							this, boost::lambda::_1),
						true))
				return 1;
		}
	}
	return 0;
}

const std::set<unsigned int>* main_window::discovered_advances() const
{
	return NULL;
}

int main_window::show_terrain_image(int x, int y, int xpos, int ypos,
		bool draw_improvements, bool draw_resources,
		bool check_resource_discovered, bool shade)
{
	return draw_terrain_tile(x, y, xpos * tile_w, ypos * tile_h, shade,
			data.m, res.terrains, res.resource_images, draw_improvements,
			draw_resources,
			check_resource_discovered ? discovered_advances() : NULL,
			screen);
}

int main_window::test_draw_border(int x, int y, int xpos, int ypos)
{
	int this_owner = data.m.get_land_owner(x, y);
	int w_owner = data.m.get_land_owner(x - 1, y);
	int e_owner = data.m.get_land_owner(x + 1, y);
	int n_owner = data.m.get_land_owner(x,     y - 1);
	int s_owner = data.m.get_land_owner(x,     y + 1);
	for(int i = 0; i < 4; i++) {
		int oth, sx, sy, ex, ey;
		switch(i) {
			case 0:
				oth = w_owner;
				sx = 0; sy = 0; ex = 0; ey = 1;
				break;
			case 1:
				oth = e_owner;
				sx = 1; sy = 0; ex = 1; ey = 1;
				break;
			case 2:
				oth = n_owner;
				sx = 0; sy = 0; ex = 1; ey = 0;
				break;
			default:
				oth = s_owner;
				sx = 0; sy = 1; ex = 1; ey = 1;
				break;
		}
		if(this_owner != oth) {
			const color& col = this_owner == -1 ? data.r.civs[oth]->col :
				data.r.civs[this_owner]->col;
			draw_line(screen, (xpos + sx) * tile_w, (ypos + sy) * tile_h, 
					(xpos + ex) * tile_w, (ypos + ey) * tile_h,
					col);
		}
	}
	return 0;
}

color main_window::get_minimap_color(int x, int y) const
{
	// minimap rectangle - no wrapping
	if((x >= cam.cam_x && x <= cam.cam_x + cam_total_tiles_x - 1) &&
	   (y >= cam.cam_y && y <= cam.cam_y + cam_total_tiles_y - 1) &&
	   (x == cam.cam_x || x == cam.cam_x + cam_total_tiles_x - 1 ||
	    y == cam.cam_y || y == cam.cam_y + cam_total_tiles_y - 1))
		return color(255, 255, 255);

	// minimap rectangle - wrapped in X
	if((cam.cam_x > data.m.wrap_x(cam.cam_x + cam_total_tiles_x - 1) && 
	    data.m.wrap_x(x) <= data.m.wrap_x(cam.cam_x + cam_total_tiles_x - 1)) &&
	   (y >= cam.cam_y && y <= cam.cam_y + cam_total_tiles_y - 1) &&
	   (x == cam.cam_x || data.m.wrap_x(x) == data.m.wrap_x(cam.cam_x + cam_total_tiles_x - 1) ||
	    y == cam.cam_y || y == cam.cam_y + cam_total_tiles_y - 1))
		return color(255, 255, 255);

	int val = data.m.get_data(x, y);
	if(val < 0 || val >= (int)res.terrains.textures.size()) {
		fprintf(stderr, "Terrain at %d not loaded at (%d, %d)\n", val,
				x, y);
		return color(0, 0, 0);
	}
	return sdl_get_pixel(res.terrains.textures[val], 16, 16);
}

int main_window::try_move_camera(bool left, bool right, bool up, bool down)
{
	bool redraw = false;
	if(left) {
		if(cam.cam_x > 0 || data.m.x_wrapped()) {
			cam.cam_x = data.m.wrap_x(cam.cam_x - 1);
			redraw = true;
		}
	}
	else if(right) {
		if(cam.cam_x < data.m.size_x() - cam_total_tiles_x || data.m.x_wrapped()) {
			cam.cam_x = data.m.wrap_x(cam.cam_x + 1);
			redraw = true;
		}
	}
	if(up) {
		if(cam.cam_y > 0 || data.m.y_wrapped()) {
			cam.cam_y = data.m.wrap_y(cam.cam_y - 1);
			redraw = true;
		}
	}
	else if(down) {
		if(cam.cam_y < data.m.size_y() - cam_total_tiles_y || data.m.y_wrapped()) {
			cam.cam_y = data.m.wrap_y(cam.cam_y + 1);
			redraw = true;
		}
	}
	if(redraw) {
		draw();
	}
	return redraw;
}

void main_window::center_camera_at(int x, int y)
{
	cam.cam_x = clamp(0, data.m.wrap_x(x - (-sidebar_size + cam_total_tiles_x) / 2), 
			data.m.size_x() - (data.m.x_wrapped() ? 0 : cam_total_tiles_x));
	cam.cam_y = clamp(0, data.m.wrap_y(y - cam_total_tiles_y / 2), 
			data.m.size_y() - (data.m.y_wrapped() ? 0 : cam_total_tiles_y));
}

void main_window::center_camera_to_unit(const unit* u)
{
	center_camera_at(u->xpos, u->ypos);
}

int main_window::try_center_camera_at(int x, int y)
{
	const int border = 3;
	if(!in_bounds(cam.cam_x + border, x, cam.cam_x - sidebar_size + cam_total_tiles_x - border) ||
	   !in_bounds(cam.cam_y + border, y, cam.cam_y + cam_total_tiles_y - border)) {
		center_camera_at(x, y);
		return true;
	}
	return false;
}

int main_window::try_center_camera_to_unit(const unit* u)
{
	return try_center_camera_at(u->xpos, u->ypos);
}

int main_window::process(int ms)
{
	return 0;
}

int main_window::handle_window_input(const SDL_Event& ev)
{
	handle_input_gui_mod(ev);
	return 0;
}

void main_window::handle_input_gui_mod(const SDL_Event& ev)
{
	switch(ev.type) {
		case SDL_KEYDOWN:
			{
				SDLKey k = ev.key.keysym.sym;
				if(k == SDLK_LEFT || k == SDLK_RIGHT || k == SDLK_UP || k == SDLK_DOWN) {
					try_move_camera(k == SDLK_LEFT, k == SDLK_RIGHT, k == SDLK_UP, k == SDLK_DOWN);
				}
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			handle_mouse_down(ev);
			break;
		case SDL_MOUSEMOTION:
			if(mouse_down_sqx >= 0)
				mouse_coord_to_tiles(ev.motion.x, ev.motion.y, &mouse_down_sqx, &mouse_down_sqy);
			break;
		case SDL_MOUSEBUTTONUP:
			handle_mouse_up(ev);
			break;
		default:
			break;
	}
}

void main_window::mouse_coord_to_tiles(int mx, int my, int* sqx, int* sqy)
{
	*sqx = (mx - sidebar_size * tile_w) / tile_w;
	*sqy = my / tile_h;
	if(*sqx >= 0) {
		*sqx = data.m.wrap_x(*sqx + cam.cam_x);
		*sqy = data.m.wrap_y(*sqy + cam.cam_y);
	}
	else {
		*sqx = -1;
		*sqy = -1;
	}
}

int main_window::handle_mouse_up(const SDL_Event& ev)
{
	mouse_down_sqx = mouse_down_sqy = -1;
	return 0;
}

int main_window::handle_mouse_down(const SDL_Event& ev)
{
	mouse_coord_to_tiles(ev.button.x, ev.button.y, &mouse_down_sqx, &mouse_down_sqy);
	return 0;
}


