#include "duckdb/main/capi_v2/capi_v2_internal.hpp"
namespace duckdb::capiv2 {
namespace {

struct CV2WindowFunction { // TODO: this should probably be a class
	Identifier name;
};

} // namespace

auto Convert(duckdb_v2_window_function_handle ptr) -> CV2WindowFunction * {
	return reinterpret_cast<CV2WindowFunction *>(ptr);
}
auto Convert(CV2WindowFunction *ptr) -> duckdb_v2_window_function_handle {
	return reinterpret_cast<duckdb_v2_window_function_handle>(ptr);
}

} // namespace duckdb::capiv2

//----------------------------------------------------------------------------------------------------------------------
// Public Functions
//----------------------------------------------------------------------------------------------------------------------

using namespace duckdb::capiv2;

DUCKDB_V2_ERROR duckdb_v2_window_function_create(duckdb_v2_factory_handle factory,
                                                 duckdb_v2_window_function_handle *function,
                                                 duckdb_v2_error_info_handle *err) {
	DUCKDB_CHECK_ARG(factory);
	DUCKDB_CHECK_ARG(function);

	return WithErrorHandler(err, [&]() {
		// TODO: see how factory is used for duckdb_v2_scalar_function_create()
		auto result = duckdb::make_uniq<CV2WindowFunction>();
		*function = Convert(result.release());
	});
}

DUCKDB_V2_ERROR duckdb_v2_window_function_set_name(duckdb_v2_window_function_handle function, const char *name,
                                                   idx_t name_length, duckdb_v2_error_info_handle *err) {
	DUCKDB_CHECK_ARG(function);
	DUCKDB_CHECK_STR_ARG(name, name_length);

	return WithErrorHandler(
	    err, [&]() { Convert(function)->name = duckdb::Identifier(ConvertIdentifierName(name, name_length)); });
}

DUCKDB_V2_ERROR duckdb_v2_window_function_destroy(duckdb_v2_window_function_handle *function) {
	return WithErrorHandler(nullptr, [&]() {
		if (!function) {
			return;
		}
		if (*function) {
			delete Convert(*function);
			*function = nullptr;
		}
	});
}
