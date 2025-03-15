#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <Psapi.h>

namespace pattern {

	struct token_t {
		uint8_t byte;
		bool is_wildcard;
	};

	uint8_t to_byte(const char _Char) {
		if (_Char >= '0' && _Char <= '9') {
			return _Char - '0';
		}
		else if (_Char >= 'A' && _Char <= 'F') {
			return _Char - 'A' + 10;
		}
		else if (_Char >= 'a' && _Char <= 'f') {
			return _Char - 'a' + 10;
		}
		return 0;
	}

	std::vector<std::string> split(const std::string& source, const std::string& delimiter, bool remove_emptry_entries = true) {
		std::vector<std::string> result;
		std::string::size_type start = 0;
		std::string::size_type end = source.find(delimiter);

		while (end != std::string::npos) {
			std::string token = source.substr(start, end - start);
			if (!remove_emptry_entries || !token.empty()) {
				result.push_back(token);
			}
			start = end + delimiter.size();
			end = source.find(delimiter, start);
		}

		std::string token = source.substr(start, end);
		if (!remove_emptry_entries || !token.empty()) {
			result.push_back(token);
		}

		return result;
	}

	// convert string to token
	// example: "8F ?? AE 1C 2D" -> { 0x8F, 0x00, 0xAE, 0x1C, 0x2D }
	std::vector<token_t> tokenize(const std::string& str) {
		auto tokens = split(str, " ");
		std::vector<token_t> bytes;
		for (const auto& token : tokens) {
			if (token == "??") {
				bytes.push_back({ 0, true });
			}
			else {
				if (token.size() != 2) {
					throw std::invalid_argument("invalid token size");
				}
				if (token[0] == '?' || token[1] == '?') {
					throw std::invalid_argument("invalid token, cannot set widcard in a value token. tips: '?A'->'AA'. ");
				}
				uint8_t byte = (to_byte(token[0]) << 4) | to_byte(token[1]);
				bytes.push_back({ byte, false });
			}
		}
		return bytes;
	}

	// search address of attribute code by pattern
	// example: search("8F ?? AE 1C 2D", first, last);
	uintptr_t search(const std::string& _Match, const uint8_t* _First, const uint8_t* _End) {

		auto tokens = tokenize(_Match);
		auto match_size = tokens.size();

		for (const uint8_t* current = _First; current < _End; ++current) {
			bool found = true;
			for (size_t i = 0; i < match_size; ++i) {
				if (tokens[i].is_wildcard) {
					continue;
				}
				if (current[i] != tokens[i].byte) {
					found = false;
					break;
				}
			}
			if (found) {
				return reinterpret_cast<uintptr_t>(current);
			}
		}

		return 0;
	}

	// search address of attribute code by pattern in current process.
	// example: search("8F ?? AE 1C 2D", GetModuleHandle(NULL)); // search address of attribute in main module.
	uintptr_t search(const std::string& _Match, HMODULE hModule) {
		MODULEINFO mi{ 0 };
		GetModuleInformation(GetCurrentProcess(), hModule, &mi, sizeof(mi));
		auto first = reinterpret_cast<uint8_t*>(hModule);
		auto last = first + mi.SizeOfImage;
		return search(_Match, first, last);
	}
}
