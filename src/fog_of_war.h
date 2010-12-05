#ifndef FOG_OF_WAR_H
#define FOG_OF_WAR_H

#include "map.h"
#include "buf2d.h"

class fog_of_war {
	public:
		fog_of_war(const map* m_);
		void reveal(int x, int y, int radius);
		void shade(int x, int y, int radius);
		char get_value(int x, int y) const;
	private:
		int get_refcount(int x, int y) const;
		int get_raw(int x, int y) const;
		void set_value(int x, int y, int val);
		void up_refcount(int x, int y);
		void down_refcount(int x, int y);
		buf2d<int> fog;
		const map* m;
};

#endif
