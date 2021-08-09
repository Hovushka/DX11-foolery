#pragma once

#include <array>
#include <string>

namespace Font {
	struct String {
		std::string data;
		std::array<int, 2> pxOffset;
		int fontSize;
	};
}