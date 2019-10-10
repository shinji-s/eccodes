// shiolab_loaders.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <vector>
#include <list>
#include <map>
#include <functional>
#include <sstream>
#include <algorithm>

#include <eccodes.h>

class ScopeGuard {
public:
	template<class Callable>
	ScopeGuard(Callable &&fn) : fn_(std::forward<Callable>(fn)) {}

	ScopeGuard(ScopeGuard &&other) : fn_(std::move(other.fn_)) {
		other.fn_ = nullptr;
	}

	~ScopeGuard() {
		// must not throw
		if (fn_) fn_();
	}

	ScopeGuard(const ScopeGuard &) = delete;
	void operator=(const ScopeGuard &) = delete;

private:
	std::function<void()> fn_;
};


// return  0 on success
// -1 on open failure
// -2 on failure to read a message from file
// -3 on unpack failure
// -4 on codes_bufr_keys_iterator_new failure
// -5 on codes_get_native_type failure
// -6 on codes_get_size failure
// (deprecated: -7 on encountering array key)
// -8 on codes_get_string failure
// -9 on codes_bufr_keys_iterator_get_name failure
// Minimum size of the buffer if the given buffer is too small

int cpp_bufr_load(const char * fname, char *buffer, size_t capacity)
{
	FILE * fp;
	if (fopen_s(&fp, fname, "rb") != 0)
		return -1;

	int err;
	codes_handle * h;
	if ((h = codes_handle_new_from_file(NULL, fp, PRODUCT_BUFR, &err)) != NULL
		|| err != CODES_SUCCESS)
	{
		if (h == NULL)
			return -2;
	}
	ScopeGuard handle_cleaner(
		[h]() { codes_handle_delete(h); });

	if (codes_set_long(h, "unpack", 1) != 0)
		return -3;

	codes_bufr_keys_iterator* kiter;
	kiter = codes_bufr_keys_iterator_new(h, 0);
	if (kiter == 0)
		return -4;
	ScopeGuard kiter_cleaner(
		[kiter]() {codes_bufr_keys_iterator_delete(kiter); });

	std::stringstream ss;
	ss << '[' << std::endl;

	std::map<std::string, int> keyname_dict;
	std::map<int, int> type_dict;
	char value_str[256];
	int section_number = 1;
	while (codes_bufr_keys_iterator_next(kiter)) {
		const char * name = codes_bufr_keys_iterator_get_name(kiter);
		if (name == 0)
			return -9;
		const char * core_name = strrchr(name, '#');
		if (core_name)
			++core_name;
		else
			core_name = name;
		// Can't handle array types at the moment.
		if (strcmp(core_name, "unexpandedDescriptors")==0)
			continue;
		if (strcmp(core_name, "subsetNumber")==0) {
			_ltoa_s(section_number++, value_str, 10);
		}
		else {
			size_t sz;
			if (codes_get_size(h, name, &sz) != 0)
				return -6;
			if (sz != 1)
				// return -7;
				continue;
			size_t len = sizeof(value_str);
			if (codes_get_string(h, name, value_str, &len) != 0)
				return -8;
		}
		auto kv = keyname_dict.find(core_name);
		int element_id;
		if (kv == keyname_dict.end()) {
			element_id = static_cast<int>(keyname_dict.size());
			std::string name_str(core_name);
			std::pair<std::string, int> pair(name_str, element_id);
			keyname_dict.insert(pair);
			int element_type;
			if (codes_get_native_type(h, core_name, &element_type) != 0)
				return -9;
			type_dict.insert(std::pair<int, int>(element_id, element_type));
		}
		else {
			element_id = kv->second;
		}
		ss << "  [" << element_id << ',';
		bool needs_quote = type_dict[element_id] == GRIB_TYPE_STRING;
		if (needs_quote)
			ss << '"';
		ss << value_str;
		if (needs_quote)
			ss << '"';
		ss << "]," << std::endl;
	}

	ss << "  [" << std::endl;
	std::list<std::pair<std::string, int>> keyname_list;
	std::transform(keyname_dict.begin(), keyname_dict.end(),
		std::back_inserter(keyname_list),
		[](const std::pair<std::string, int> &pair) { return pair; });
	keyname_list.sort([](
		const std::pair<std::string, int>& a,
		const std::pair<std::string, int>& b) {
		return a.second < b.second;
	});
	bool first = true;
	for (const auto& kv : keyname_list) {
		if (!first)
			ss << ',' << std::endl;
		else
			first = false;
		ss << "    [" << kv.second << ",\"" << kv.first << "\"]";
	}
	ss << std::endl << "  ]" << std::endl;
	ss << ']' << std::endl;
	std::string output = ss.str();
	if (output.size() < capacity) {
		memcpy(buffer, output.c_str(), output.size());
		buffer[output.size()] = '\0';
		return 0;
	}
	// buffer is not big enough.
	// return the minimum size of the buffer required.
	return static_cast<long>(output.size() + 1);
}

extern "C" {
	__declspec(dllexport)
	int bufr_load(const char * fname, char * buffer, size_t capacity)
	{
		int r;
		r = cpp_bufr_load(fname, buffer, capacity);
		return r;
	}
}
