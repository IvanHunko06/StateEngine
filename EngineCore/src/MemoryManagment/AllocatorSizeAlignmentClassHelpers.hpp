#pragma once
#include <cmath>
#include <algorithm>
#include <bit>
namespace StateEngine::EngineCore::MemoryManagment {
	constexpr int32_t getClassFromSize(size_t sz, size_t maxSize, size_t powerOfTwoUpperBound, size_t powerOfTwoLowerBound) {
		if (sz >= maxSize)
			return -1; // Large allocation

		if (!sz)
			return -1; // Invalid size

		if (sz <= powerOfTwoUpperBound) {
			size_t curSize = powerOfTwoLowerBound;
			int32_t i = 0;
			while (curSize < sz) {
				curSize <<= 1;
				++i;
			}
			return i;
		}


		// For sizes larger than kPowerOfTwoUpperBound and less than kLargeAllocThreshold
		size_t base = powerOfTwoUpperBound;
		int32_t cls = static_cast<int32_t>(std::bit_width(powerOfTwoUpperBound / powerOfTwoLowerBound) - 1);

		while (true) {
			size_t step = std::max<size_t>(32, base / 32);
			base = (std::min)(base + step, maxSize);
			++cls;
			if (sz <= base)
				return cls;
			else if (base >= maxSize)
				break;
		}
		return -1; // Should not reach here
	}
	constexpr size_t getSizeFromClass(int32_t cls, size_t maxSize, size_t powerOfTwoUpperBound, size_t powerOfTwoLowerBound) {
		if (cls < 0)
			return 0; // Invalid size class

		int32_t powerOfTwoCount = static_cast<int32_t>(
			std::bit_width(powerOfTwoUpperBound /powerOfTwoLowerBound) - 1
			);

		if (cls <= powerOfTwoCount) {
			return powerOfTwoLowerBound << cls;
		}
		size_t base = powerOfTwoUpperBound;
		int32_t curCls = powerOfTwoCount;
		while (true) {
			size_t step = std::max<size_t>(32, base / 32);
			base = (std::min)(base + step, maxSize);
			++curCls;
			if (curCls == cls)
				return base;
			else if (base >= maxSize)
				break;
		}
		return 0; // Invalid size class
	}
	constexpr int32_t getClassFromAlignment(size_t alignment) {
		switch (alignment)
		{
		case 1:
		case 2:
		case 4:
		case 8:
		case 16:
			return 0;
		case 32:
			return 1;
		case 64:
			return 2;
		default:
			return -1;
		}
	}
	constexpr size_t getAlignmentFromClass(int32_t cls) {
		switch (cls)
		{
		case 0:
			return 16;
		case 1:
			return 32;
		case 2:
			return 64;
		default:
			return 0;
		}
	}
}