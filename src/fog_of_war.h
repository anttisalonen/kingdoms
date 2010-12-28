#ifndef FOG_OF_WAR_H
#define FOG_OF_WAR_H

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "map.h"
#include "buf2d.h"

class fog_of_war {
	public:
		fog_of_war(const map* m_);
		fog_of_war(); // for serialization
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

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & fog;
			ar & const_cast<map*&>(m);
		}
};

#endif
