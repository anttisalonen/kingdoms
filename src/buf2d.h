#ifndef BUF2D_H
#define BUF2D_H

template<typename N>
class buf2d {
	public:
		buf2d(int x, int y);
		~buf2d();
		const N* get(int x, int y) const;
		void set(int x, int y, const N& val);
		const int size_x;
		const int size_y;
	private:
		int get_index(int x, int y) const;
		N* data;
};

template<typename N>
buf2d<N>::buf2d(int x, int y)
	: size_x(x),
	size_y(y)
{
	this->data = new N[x * y];
}

template<typename N>
buf2d<N>::~buf2d()
{
	delete[] this->data;
}

template<typename N>
int buf2d<N>::get_index(int x, int y) const
{
	return y * size_x + x;
}

template<typename N>
void buf2d<N>::set(int x, int y, const N& val)
{
	if(!in_bounds(0, x, size_x - 1) || !in_bounds(0, y, size_y - 1))
		return;
	data[get_index(x, y)] = val;
}

template<typename N>
const N* buf2d<N>::get(int x, int y) const
{
	if(!in_bounds(0, x, size_x - 1) || !in_bounds(0, y, size_y - 1))
		return NULL;
	return &data[get_index(x, y)];
}

#endif

