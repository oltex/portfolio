#pragma once
#include "../design-pattern/singleton.h"
#include "../utility/parser.h"
#include <string>
#include <functional>

class command final : public design_pattern::singleton<command> {
private:
	friend class design_pattern::singleton<command>;
	using size_type = unsigned int;
public:
	class parameter final {
	public:
		template<typename... argument>
		inline explicit parameter(argument&&... arg) noexcept {
			(_argument.emplace_back(std::forward<argument>(arg)), ...);
		}
		inline explicit parameter(data_structure::vector<std::string>& argument) noexcept
			: _argument(argument) {
		}
		inline explicit parameter(parameter const& rhs) noexcept = delete;
		inline explicit parameter(parameter&& rhs) noexcept = delete;
		inline auto operator=(parameter const& rhs) noexcept -> parameter & = delete;
		inline auto operator=(parameter&& rhs) noexcept -> parameter & = delete;
		inline ~parameter(void) noexcept = default;
	public:
		inline auto get_string(size_type index) noexcept -> std::string& {
			return _argument[index];
		}
		inline auto get_int(size_type index) noexcept -> int {
			return std::stoi(_argument[index]);
		}
		inline auto get_float(size_type index) noexcept -> float {
			return std::stof(_argument[index]);
		}
		inline auto get_bool(size_type index) noexcept -> bool {
			std::string& arg = _argument[index];
			if (arg == "true" || arg == "on" || arg == "1")
				return true;
			return false;
		}
		inline auto size(void) const noexcept -> size_type {
			return _argument.size();
		}
	private:
		data_structure::vector<std::string> _argument;
	};
private:
	inline explicit command(void) noexcept {
		add("include", [&](command::parameter* param) noexcept -> int {
			std::string path = param->get_string(1);
			utility::parser parser(std::wstring(path.begin(), path.end()));

			for (auto& iter : parser) {
				command::parameter param(iter);
				execute(iter[0], &param);
			}
			return 0;
			});
	};
	inline explicit command(command const& rhs) noexcept = delete;
	inline explicit command(command&& rhs) noexcept = delete;
	inline auto operator=(command const& rhs) noexcept -> command & = delete;
	inline auto operator=(command&& rhs) noexcept -> command & = delete;
	inline ~command(void) noexcept = default;
public:
	inline void add(std::string_view name, std::function<int(parameter*)> function) noexcept {
		_function.emplace(name.data(), function);
	}
	inline int execute(std::string name, parameter* par) noexcept {
		auto res = _function.find(name.data());
		if (res == _function.end())
			return 0;

		return res->second(par);
	}
public:
	std::unordered_map<std::string, std::function<int(parameter*)>> _function;
};