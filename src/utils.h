#ifndef UTILS_H
#define UTILS_H

#define CALL_MEMBER_FUN(object,ptrToMember) ((object).*(ptrToMember))

bool in_bounds(int a, int b, int c);

template<typename N>
N clamp(N min, N val, N max)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}

#endif
