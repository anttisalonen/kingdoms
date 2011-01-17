#include "resource.h"
#include "resource_configuration.h"

bool resource::allowed_on(unsigned int t) const
{
	t++;
	for(unsigned int i = 0; i < max_num_resource_terrains; i++) {
		if(terrain[i] == t)
			return true;
	}
	return false;
}

