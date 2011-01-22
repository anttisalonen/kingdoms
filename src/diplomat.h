#ifndef DIPLOMAT_H
#define DIPLOMAT_H

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

class diplomat {
	public:
		virtual ~diplomat() { }
		virtual bool peace_suggested(int civ_id) = 0;

	private:
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
		}
};

#endif

