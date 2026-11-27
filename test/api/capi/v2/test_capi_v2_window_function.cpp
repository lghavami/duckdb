#include "test_capi_v2.hpp"

#include <string>

namespace test_capi_v2 {

namespace {

// Create a window function on the connection with the given name.
duckdb_v2_window_function_handle MakeWindow(duckdb_v2_connection_handle conn, std::string_view name) {
	duckdb_v2_window_function_handle function = nullptr;
	REQUIRE(duckdb_v2_window_function_create(FactoryOf(conn), &function, nullptr) == DUCKDB_V2_ERROR_NONE);
	REQUIRE(duckdb_v2_window_function_set_name(function, name.data(), name.size(), nullptr) == DUCKDB_V2_ERROR_NONE);
	return function;
}

// The text of an error info, or an empty string if there is none.
std::string WindowErrorText(duckdb_v2_error_info_handle err) {
	const char *msg = nullptr;
	idx_t msg_length = 0;
	if (!err || duckdb_v2_error_info_get_text(err, &msg, &msg_length) != DUCKDB_V2_ERROR_NONE || !msg) {
		return std::string();
	}
	return std::string(msg, msg_length);
}

} // namespace

TEST_CASE("V2 window: null arguments and destroy null-safety", "[capi_v2][window_function]") {
	EnvFixture fx;

	duckdb_v2_window_function_handle function = nullptr;
	REQUIRE(duckdb_v2_window_function_create(nullptr, &function, nullptr) == DUCKDB_V2_ERROR_INPUT_INVALID);
	REQUIRE(function == nullptr);
	REQUIRE(duckdb_v2_window_function_create(fx.factory, nullptr, nullptr) == DUCKDB_V2_ERROR_INPUT_INVALID);

	REQUIRE(duckdb_v2_window_function_create(fx.factory, &function, nullptr) == DUCKDB_V2_ERROR_NONE);
	REQUIRE(duckdb_v2_window_function_set_name(function, nullptr, 1, nullptr) == DUCKDB_V2_ERROR_INPUT_INVALID);
	std::string_view invalid_name = "\x80";
	REQUIRE(duckdb_v2_window_function_set_name(function, invalid_name.data(), invalid_name.size(), nullptr) ==
	        DUCKDB_V2_ERROR_INPUT_INVALID);

	REQUIRE(duckdb_v2_window_function_destroy(&function) == DUCKDB_V2_ERROR_NONE);
	REQUIRE(function == nullptr);

	REQUIRE(duckdb_v2_window_function_destroy(nullptr) == DUCKDB_V2_ERROR_NONE);
	duckdb_v2_window_function_handle null_function = nullptr;
	REQUIRE(duckdb_v2_window_function_destroy(&null_function) == DUCKDB_V2_ERROR_NONE);
}

TEST_CASE("V2 window: set_name", "[capi_v2][window_function]") {
	EnvFixture fx;
	auto function = MakeWindow(fx.conn, "my_window");

	SECTION("valid names are accepted") {
		for (std::string_view name : {"w", "MixedCase", "with space", "with \"quote\"", "f\xC3\xBCnf"}) {
			REQUIRE(duckdb_v2_window_function_set_name(function, name.data(), name.size(), nullptr) ==
			        DUCKDB_V2_ERROR_NONE);
		}
	}

	SECTION("the length bounds the name, not a null terminator") {
		std::string_view padded = "my_window_with_suffix";
		REQUIRE(duckdb_v2_window_function_set_name(function, padded.data(), 9, nullptr) == DUCKDB_V2_ERROR_NONE);
		// An invalid byte past the length is never read.
		std::string_view trailing = "ok\x80";
		REQUIRE(duckdb_v2_window_function_set_name(function, trailing.data(), 2, nullptr) == DUCKDB_V2_ERROR_NONE);
	}

	SECTION("setting the name again succeeds") {
		std::string_view first = "first";
		std::string_view second = "second";
		REQUIRE(duckdb_v2_window_function_set_name(function, first.data(), first.size(), nullptr) ==
		        DUCKDB_V2_ERROR_NONE);
		REQUIRE(duckdb_v2_window_function_set_name(function, second.data(), second.size(), nullptr) ==
		        DUCKDB_V2_ERROR_NONE);
	}

	SECTION("an empty name is accepted by set_name") {
		REQUIRE(duckdb_v2_window_function_set_name(function, "", 0, nullptr) == DUCKDB_V2_ERROR_NONE);
		// A null pointer with length 0 is an empty string, not a null argument.
		REQUIRE(duckdb_v2_window_function_set_name(function, nullptr, 0, nullptr) == DUCKDB_V2_ERROR_NONE);
	}

	SECTION("a null handle is rejected") {
		std::string_view name = "my_window";
		duckdb_v2_error_info_handle err = nullptr;
		REQUIRE(duckdb_v2_window_function_set_name(nullptr, name.data(), name.size(), &err) ==
		        DUCKDB_V2_ERROR_INPUT_INVALID);
		REQUIRE(err != nullptr);
		REQUIRE(WindowErrorText(err).find("function") != std::string::npos);
		duckdb_v2_error_info_destroy(&err);
	}

	SECTION("a null name with a length is rejected") {
		duckdb_v2_error_info_handle err = nullptr;
		REQUIRE(duckdb_v2_window_function_set_name(function, nullptr, 3, &err) == DUCKDB_V2_ERROR_INPUT_INVALID);
		REQUIRE(err != nullptr);
		REQUIRE(WindowErrorText(err).find("name") != std::string::npos);
		duckdb_v2_error_info_destroy(&err);
	}

	SECTION("invalid UTF-8 is rejected") {
		duckdb_v2_error_info_handle err = nullptr;
		for (std::string_view name : {"\x80", "abc\xFF", "\xC3"}) {
			REQUIRE(duckdb_v2_window_function_set_name(function, name.data(), name.size(), &err) ==
			        DUCKDB_V2_ERROR_INPUT_INVALID);
			REQUIRE(WindowErrorText(err).find("UTF-8") != std::string::npos);
		}
		duckdb_v2_error_info_destroy(&err);
		// A rejected name leaves the function usable.
		std::string_view name = "after_error";
		REQUIRE(duckdb_v2_window_function_set_name(function, name.data(), name.size(), nullptr) ==
		        DUCKDB_V2_ERROR_NONE);
	}

	REQUIRE(duckdb_v2_window_function_destroy(&function) == DUCKDB_V2_ERROR_NONE);
	REQUIRE(function == nullptr);
}

} // namespace test_capi_v2
