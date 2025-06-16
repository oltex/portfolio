#pragma once

#include "camera.h"
#include "a-star/grid.h"
#include "a-star/path.h"

#include <string>

class recode final {
public:
	inline explicit recode(void) noexcept {
		_font = CreateFontW(18, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE,
			L"Malgun Gothic");
	}
	inline ~recode(void) noexcept = default;
public:
	inline void paint(HDC hdc, a_star::path& path) noexcept {
		SelectObject(hdc, _font);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(255, 255, 255));
		if (!path._result.empty()) {
			auto iter = path._result.begin();
			auto iter2 = path._result.begin();
			iter2++;
			float distance = 0.f;
			for (int i = 0; i != path._result.size() - 1; ++i, ++iter, ++iter2)
				distance += (*iter).distance_euclidean((*iter2));

			std::wstring wstr = L"distance : " + std::to_wstring(distance);
			TextOutW(hdc, 0, 0, wstr.c_str(), (int)wstr.size());
		}
	}
private:
	HGDIOBJ _font;
};