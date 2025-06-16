#pragma once
#include "../design-pattern/singleton.h"
#include "../data-structure/pair.h"
#include "../data-structure/unique_pointer.h"
#include <Pdh.h>
#pragma comment(lib,"pdh.lib")
#include <string>
#include <list>
#include <iostream>

namespace utility::performance_data_helper {
	class query final : public design_pattern::singleton<query> {
	public:
		class counter final {
		public:
			using value = PDH_FMT_COUNTERVALUE;
			using value_item = PDH_FMT_COUNTERVALUE_ITEM_W;
		public:
			inline explicit counter(void) noexcept = default;
			inline counter(counter const&) noexcept = default;
			inline explicit counter(counter&& rhs) noexcept
				: _counter(rhs._counter) {
				rhs._counter = INVALID_HANDLE_VALUE;
			};
			inline auto operator=(counter const&) noexcept -> counter & = default;
			inline auto operator=(counter&& rhs) noexcept -> counter& {
				_counter = rhs._counter;
				rhs._counter = INVALID_HANDLE_VALUE;
				return *this;
			};
			inline ~counter(void) noexcept {
				PdhRemoveCounter(_counter);
			};
		public:
			inline auto get_formatted_value(unsigned long format) noexcept -> value {
				value counter_value_;
				PdhGetFormattedCounterValue(_counter, format, nullptr, &counter_value_);
				return counter_value_;
			}
			inline auto get_formatted_array(unsigned long format) noexcept -> data_structure::pair<unsigned long, data_structure::unique_pointer<value_item[]>> {
				unsigned long buffer_size = 0;
				unsigned long item_count;
				PdhGetFormattedCounterArrayW(_counter, format, &buffer_size, &item_count, nullptr);
				value_item* counter_value_item_ = reinterpret_cast<value_item*>(malloc(buffer_size));
				PdhGetFormattedCounterArrayW(_counter, format, &buffer_size, &item_count, counter_value_item_);

				return data_structure::pair<unsigned long, data_structure::unique_pointer<value_item[]>>(item_count, counter_value_item_);
			}
			inline auto data(void) noexcept -> PDH_HCOUNTER& {
				return _counter;
			}
		private:
			PDH_HCOUNTER _counter;
		};
	private:
		friend class design_pattern::singleton<query>;
	private:
		inline explicit query(void) noexcept {
			PdhOpenQueryW(nullptr, NULL, &_qurey);
		};
		inline explicit query(query const&) noexcept = delete;
		inline explicit query(query&&) noexcept = delete;
		inline auto operator=(query const&) noexcept -> query & = delete;
		inline auto operator=(query&&) noexcept -> query & = delete;
		inline ~query(void) noexcept {
			PdhCloseQuery(&_qurey);
		}
	public:
		inline auto add_counter(std::wstring_view const object, std::wstring_view const item, std::wstring_view const instance) noexcept -> counter {
			size_t size = sizeof(wchar_t) * (object.size() + item.size() + instance.size() + 7);
			wchar_t* path = reinterpret_cast<wchar_t*>(malloc(size));
#pragma warning(suppress: 6387)
			swprintf_s(path, size / sizeof(wchar_t), L"\\%s(%s)\\%% %s", object.data(), instance.data(), item.data());
			counter counter_;
#pragma warning(suppress: 6387)
			PdhAddCounterW(_qurey, path, NULL, &counter_.data());
			free(path);
			return counter_;
		}
		inline auto add_counter(std::wstring_view const object, std::wstring_view const item) noexcept -> counter {
			size_t size = object.size() + item.size() + 5;
			wchar_t* path = reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t) * size));
#pragma warning(suppress: 6387)
			swprintf_s(path, size / sizeof(wchar_t), L"\\%s\\%% %s", object.data(), item.data());
			counter counter_;
#pragma warning(suppress: 6387)
			PdhAddCounterW(_qurey, path, NULL, &counter_.data());
			free(path);
			return counter_;
		}
		inline auto add_counter(std::wstring_view const path) noexcept -> counter {
			counter counter_;
			PdhAddCounterW(_qurey, path.data(), NULL, &counter_.data());
			return counter_;
		}
		inline void collect_query_data(void) noexcept {
			PdhCollectQueryData(_qurey);
		}
		inline void collect_query_data_ex(void) noexcept {
			//PdhCollectQueryDataEx()
		}
		inline auto expand_counter_path(std::wstring_view const path) noexcept {
		}
	private:
		PDH_HQUERY _qurey;
	};

	inline static auto enum_object(void) noexcept -> std::list<std::wstring> {
		unsigned long object_size = 0;
		PdhEnumObjectsW(nullptr, nullptr, nullptr, &object_size, PERF_DETAIL_WIZARD, TRUE);
		wchar_t* object_buffer = reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t) * object_size));
		PdhEnumObjectsW(nullptr, nullptr, object_buffer, &object_size, PERF_DETAIL_WIZARD, FALSE);
		wchar_t const* object = object_buffer;

		std::list<std::wstring> result;
#pragma warning(suppress: 6011)
		for (wchar_t const* object = object_buffer; *object != false; object += wcslen(object) + 1)
			result.emplace_back(object);
		free(object_buffer);

		result.sort();
		return result;
	}
	inline static auto enum_item_instance(std::wstring_view const object) noexcept -> std::pair<std::list<std::wstring>, std::list<std::wstring>> {
		unsigned long item_size = 0;
		unsigned long instance_size = 0;
		PdhEnumObjectItemsW(nullptr, nullptr, object.data(), nullptr, &item_size, nullptr, &instance_size, PERF_DETAIL_WIZARD, 0);
		wchar_t* item_buffer = reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t) * item_size));
		wchar_t* instance_buffer = reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t) * instance_size));
		PdhEnumObjectItemsW(nullptr, nullptr, object.data(), item_buffer, &item_size, instance_buffer, &instance_size, PERF_DETAIL_WIZARD, 0);

		std::pair<std::list<std::wstring>, std::list<std::wstring>> result;
#pragma warning(suppress: 6011)
		for (wchar_t const* item = item_buffer; *item != false; item += wcslen(item) + 1)
			result.first.emplace_back(item);
#pragma warning(suppress: 6011)
		for (wchar_t const* instance = instance_buffer; *instance != false; instance += wcslen(instance) + 1)
			result.second.emplace_back(instance);

		free(item_buffer);
		free(instance_buffer);

		result.first.sort();
		result.second.sort();
		return result;
	}
}