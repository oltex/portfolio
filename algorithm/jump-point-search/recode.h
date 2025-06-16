#pragma once

#include "camera.h"
#include "jump-point-search/grid.h"
#include "jump-point-search/path.h"

#include "../../system/window/device_context.h"
#include "../../system/window/font.h"
#include <string>

class recode final {
public:
	inline explicit recode(void) noexcept
		: _font(18, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE,
			L"Malgun Gothic") {

	}
	inline ~recode(void) noexcept = default;
public:
	inline void paint(window::device_context& dc, algorithm::jump_point_search::path& path) noexcept {
		dc.select_object(_font);
		dc.set_bk_mode(TRANSPARENT);
		dc.set_text_color(RGB(255, 255, 255));
		if (!path._result.empty()) {
			auto iter = path._result.begin();
			auto iter2 = path._result.begin();
			iter2++;
			float distance = 0.f;
			for (int i = 0; i != path._result.size() - 1; ++i, ++iter, ++iter2)
				distance += (*iter).distance_euclidean((*iter2));

			std::wstring wstr = L"distance : " + std::to_wstring(distance);
			dc.text_out(0, 0, wstr.c_str(), (int)wstr.size());
		}
	}
private:
	window::font _font;
};