#pragma once
#include "../system-component/file.h"
#include "unordered_map"
#include <string>

namespace utility {
	class parser final {
		using size_type = unsigned int;
	public:
		inline explicit parser(void) noexcept = default;
		inline explicit parser(std::wstring_view path) noexcept {
			open(path);
		}
		inline explicit parser(parser const& rhs) noexcept = delete;
		inline explicit parser(parser&& rhs) noexcept = delete;
		inline auto operator=(parser const& rhs) noexcept -> parser & = delete;
		inline auto operator=(parser&& rhs) noexcept -> parser & = delete;
		inline ~parser(void) noexcept = default;
	public:
		inline void open(std::wstring_view path) noexcept {
			system_component::file file(path, GENERIC_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
			auto size = file.get_size_ex().QuadPart;

			char* buffer = reinterpret_cast<char*>(malloc(sizeof(char) * size + 1));
			file.read(buffer, static_cast<unsigned long>(size));
#pragma warning(suppress: 6011)
			buffer[size] = 0;

			data_structure::vector<std::string> item;
			static constexpr char comment_char = '#';
			static constexpr char split_char[] = " ,:(){}[]=";
			static constexpr char end_char[] = { '\r', '\n', ';', 0 };
			bool encoding = true;

			char* current = buffer;
			size_type begin_index = 0;
			for (size_type index = 0; index < size + 1; ++index, ++current) {
				bool finish = false;

				// comment
#pragma warning(suppress: 6011)
				if (*current == comment_char) {
					encoding = false;
					finish = true;
				}
				// end char
				if (false == finish) {
					for (auto& iter : end_char) {
						if (*current == iter) {
							if (true == encoding && begin_index != index)
								item.emplace_back(std::string(buffer + begin_index, index - begin_index));
							if (!item.empty()) {
								_parsing.emplace_back(item);
								item.clear();
							}
							begin_index = index + 1;
							encoding = true;
							finish = true;
							break;
						}
					}
				}
				// split char
				if (false == finish && true == encoding) {
					for (auto& iter : split_char) {
						if (*current == iter) {
							if (begin_index != index)
								item.emplace_back(std::string(buffer + begin_index, index - begin_index));
							begin_index = index + 1;
							finish = true;
							break;
						}
					}
				}
			}
			free(buffer);
		}
		inline auto begin(void) noexcept {
			return _parsing.begin();
		}
		inline auto end(void) noexcept {
			return _parsing.end();
		}
	private:
		std::list<data_structure::vector<std::string>> _parsing;
	};
}