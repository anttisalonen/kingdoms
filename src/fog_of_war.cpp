#include "fog_of_war.h"

fog_of_war::fog_of_war(const map* m_)
	: fog(buf2d<int>(m_ ? m_->size_x() : 0, m_ ? m_->size_y() : 0, 0)), m(m_)
{
}

void fog_of_war::reveal(int x, int y, int radius)
{
	for(int i = x - radius; i <= x + radius; i++) {
		for(int j = y - radius; j <= y + radius; j++) {
			int di = m->wrap_x(i);
			int dj = m->wrap_y(j);
			up_refcount(di, dj);
			set_value(di, dj, 2);
		}
	}
}

void fog_of_war::shade(int x, int y, int radius)
{
	for(int i = x - radius; i <= x + radius; i++) {
		for(int j = y - radius; j <= y + radius; j++) {
			int di = m->wrap_x(i);
			int dj = m->wrap_y(j);
			down_refcount(di, dj);
			if(get_refcount(di, dj) == 0)
				set_value(di, dj, 1);
		}
	}
}

char fog_of_war::get_value(int x, int y) const
{
	return get_raw(x, y) & 3;
}

int fog_of_war::get_refcount(int x, int y) const
{
	return get_raw(x, y) >> 2;
}

int fog_of_war::get_raw(int x, int y) const
{
	const int* i = fog.get(x, y);
	if(!i)
		return 0;
	return *i;
}

void fog_of_war::up_refcount(int x, int y)
{
	int i = get_refcount(x, y);
	int v = get_value(x, y);
	fog.set(x, y, ((i + 1) << 2) | v);
}

void fog_of_war::down_refcount(int x, int y)
{
	int i = get_refcount(x, y);
	if(i == 0)
		return;
	int v = get_value(x, y);
	fog.set(x, y, ((i - 1) << 2) | v);
}

void fog_of_war::set_value(int x, int y, int val)
{
	int i = get_raw(x, y);
	i &= ~3;
	fog.set(x, y, i | val); 
}


