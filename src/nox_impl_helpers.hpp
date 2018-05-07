#pragma once

#include <vector>

namespace nf {

	template<class C>
	void ArrayToUnrealPtr(size_t & size, C *& ptr, std::vector<C> &data) {
		size = data.size();
		ptr = size > 0 ? &data[0] : nullptr;
	}
}