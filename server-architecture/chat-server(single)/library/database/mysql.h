#pragma once
//#pragma comment(lib, "library/mysqlclient.lib")
#include "include/mysql.h"
#include "include/mysqld_error.h"
#include "include/errmsg.h"
#include <iostream>

namespace database {
	class mysql final {
	public:
		class result final {
		private:
			class row final {
			private:
				using size_type = unsigned int;
			public:
				inline explicit row(void) noexcept
					: _mysql_row(nullptr) {
				};
				inline explicit row(MYSQL_ROW  mysql_row) noexcept
					: _mysql_row(mysql_row) {
				};
				inline explicit row(row const&) noexcept = delete;
				inline explicit row(row&& rhs) noexcept
					: _mysql_row(rhs._mysql_row) {
				};
				inline auto operator=(row const&) noexcept -> row & = delete;
				inline auto operator=(row&& rhs) noexcept -> row& {
					_mysql_row = rhs._mysql_row;
					return *this;
				};
				inline ~row(void) noexcept = default;

				inline auto get_char_pointer(size_type index) noexcept -> char* {
					return _mysql_row[index];
				}
				inline auto get_int(size_type index) noexcept -> int {
					return std::atoi(_mysql_row[index]);
				}
				inline auto get_unsigned_long_long(size_type index) noexcept -> unsigned long long {
					return std::strtoull(_mysql_row[index], nullptr, 10);
				}
				inline auto get_float(size_type index) noexcept -> float {
					return static_cast<float>(std::atof(_mysql_row[index]));
				}
				inline auto get_double(size_type index) noexcept -> double {
					return std::atof(_mysql_row[index]);
				}
				inline auto get_bool(size_type index) noexcept -> bool {
					if ("1" == _mysql_row[index])
						return true;
					return false;
				}
				inline bool operator==(row const& rhs) const noexcept {
					return _mysql_row == rhs._mysql_row;
				}
				inline bool operator!=(row const& rhs) const noexcept {
					return _mysql_row != rhs._mysql_row;
				}
			private:
				MYSQL_ROW _mysql_row;
			};
			using field = MYSQL_FIELD;
			using field_offest = MYSQL_FIELD_OFFSET;
			using row_offset = MYSQL_ROW_OFFSET;
		public:
			class iterator final {
			public:
				inline explicit iterator(void) noexcept
					: _mysql_row(), _mysql_result(nullptr) {
				};
				inline explicit iterator(MYSQL_RES* mysql_result) noexcept
					: _mysql_result(mysql_result), _mysql_row(mysql_fetch_row(_mysql_result)) {
				};
				inline explicit iterator(iterator const&) noexcept = delete;
				inline explicit iterator(iterator&&) noexcept = delete;
				inline auto operator=(iterator const&) noexcept -> iterator & = delete;
				inline auto operator=(iterator&&) noexcept -> iterator & = delete;
				inline ~iterator(void) noexcept = default;

				inline auto operator*(void) noexcept -> row& {
					return _mysql_row;
				}
				inline auto operator->(void) noexcept -> row* {
					return &_mysql_row;
				}
				inline auto operator++(void) noexcept -> iterator& {
					_mysql_row = row(mysql_fetch_row(_mysql_result));
					return *this;
				}
				inline bool operator==(iterator const& rhs) const noexcept {
					return _mysql_row == rhs._mysql_row;
				}
				inline bool operator!=(iterator const& rhs) const noexcept {
					return _mysql_row != rhs._mysql_row;
				}
			private:
				MYSQL_RES* _mysql_result;
				row _mysql_row;
			};
		public:
			inline explicit result(MYSQL_RES* mysql_result) noexcept
				: _mysql_result(mysql_result) {
			};
			inline explicit result(result const&) noexcept = delete;
			inline explicit result(result&&) noexcept = delete;
			inline auto operator=(result const&) noexcept -> result & = delete;
			inline auto operator=(result&&) noexcept -> result & = delete;
			inline ~result(void) noexcept {
				mysql_free_result(_mysql_result);
			};

			inline auto fetch_field(void) noexcept -> field* {
				return mysql_fetch_fields(_mysql_result);
			}
			inline auto num_field(void) noexcept -> unsigned int {
				return mysql_num_fields(_mysql_result);
			}
			inline auto field_tell(void) noexcept -> field_offest {
				return mysql_field_tell(_mysql_result);
			}
			inline auto field_seek(field_offest offset) noexcept -> field_offest {
				return mysql_field_seek(_mysql_result, offset);
			}

			inline auto fetch_row(void) noexcept -> row {
				return row(mysql_fetch_row(_mysql_result));
			}
			inline auto row_tell(void) noexcept -> row_offset {
				return mysql_row_tell(_mysql_result);
			}
			inline auto row_seek(row_offset offset) noexcept -> row_offset {
				return mysql_row_seek(_mysql_result, offset);
			}
			inline auto num_row(void) noexcept -> unsigned long long {
				return mysql_num_rows(_mysql_result);
			}

			inline auto fetch_length(void) noexcept -> unsigned long* {
				return mysql_fetch_lengths(_mysql_result);
			}
			inline void data_seek(unsigned long long offset) noexcept {
				mysql_data_seek(_mysql_result, offset);
			}

			inline auto begin(void) noexcept -> iterator {
				return iterator(_mysql_result);
			}
			inline auto end(void) noexcept -> iterator {
				return iterator();
			}
		private:
			MYSQL_RES* _mysql_result;
		};
	public:
		inline explicit mysql(void) noexcept {
			mysql_init(&_mysql);
		};
		inline explicit mysql(mysql const&) noexcept = delete;
		inline explicit mysql(mysql&&) noexcept = delete;
		inline auto operator=(mysql const&) noexcept -> mysql & = delete;
		inline auto operator=(mysql&&) noexcept -> mysql & = delete;
		inline ~mysql(void) noexcept {
			mysql_close(&_mysql);
		};

		inline void connect(char const* host, char const* user, char const* password, char const* db, unsigned int port) noexcept {
			if (nullptr == mysql_real_connect(&_mysql, host, user, password, db, port, nullptr, 0)) {
				auto error = mysql_errno(&_mysql);
				switch (error) {
				case ER_ACCESS_DENIED_ERROR:
				case ER_BAD_DB_ERROR:
				case CR_CONN_HOST_ERROR:
				default:
					__debugbreak();
				}
			}
		}
		inline void begin(void) noexcept {
			query("begin");
		}
		inline void commit(void) noexcept {
			query("commit");
		}
		inline void rollback(void) noexcept {
			query("rollback");
		}
		inline void use(char const* const schema) noexcept {
			query("use %d", schema);
		}
		inline void truncate_table(char const* const name) noexcept {
			query("truncate table %s", name);
		}
		inline void query(char const* const format, ...) noexcept {
			char buffer[256]{};
			va_list va_list_;
			va_start(va_list_, format);
			int length = _vsnprintf_s(buffer, 255, 255, format, va_list_);
			va_end(va_list_);
			if (0 != mysql_query(&_mysql, buffer)) {
				auto result = mysql_errno(&_mysql);
				switch (result) {
				case ER_PARSE_ERROR:
				case CR_SERVER_LOST:
				default:
					__debugbreak();
				}
			}
		}

		inline auto store_result(void) noexcept -> result {
			return result(mysql_store_result(&_mysql));
		}
		inline auto use_result(void) noexcept {
			return result(mysql_use_result(&_mysql));
		}
		inline auto insert_id(void) noexcept -> unsigned long long {
			return mysql_insert_id(&_mysql);
		}
		inline auto ping(void) noexcept -> int {
			return mysql_ping(&_mysql);
		}

		inline static void initialize(void) noexcept {
			mysql_server_init(0, nullptr, nullptr);
		}
		inline static void end(void) noexcept {
			mysql_server_end();
		}
	private:
		MYSQL _mysql;
	};
}