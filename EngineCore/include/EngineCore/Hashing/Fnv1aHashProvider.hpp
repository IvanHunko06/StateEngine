#pragma once
#include <cstdint>
#include <type_traits>

namespace StateEngine::EngineCore::Hashing {

	class Fnv1aHashProvider {
	public:

		Fnv1aHashProvider() = delete;
		Fnv1aHashProvider(const Fnv1aHashProvider&) = delete;
		Fnv1aHashProvider(Fnv1aHashProvider&&) = delete;
		Fnv1aHashProvider& operator=(const Fnv1aHashProvider&) = delete;
		Fnv1aHashProvider& operator=(Fnv1aHashProvider&&) = delete;

		static size_t hashBytes(const void* data, size_t lenght) {
			size_t hash = 0;
			size_t fnvPrime = 0;
			if constexpr (sizeof(size_t) == 8) {
				hash = 14695981039346656037;
				fnvPrime = 1099511628211;
			}
			else {
				hash = 2166136261;
				fnvPrime = 16777619;
			}

			const uint8_t* currentByte = reinterpret_cast<const uint8_t*>(data);

			for (size_t i = 0; i < lenght; ++i) {
				hash = hash ^ *currentByte;
				hash = hash * fnvPrime;
				++currentByte;
			}
			return hash;
		}
		consteval static size_t hashString(const char* str) {
			size_t hash = 0;
			size_t fnvPrime = 0;
			if constexpr (sizeof(size_t) == 8) {
				hash = 14695981039346656037;
				fnvPrime = 1099511628211;
			}
			else {
				hash = 2166136261;
				fnvPrime = 16777619;
			}
			size_t index = 0;
			while (str[index++] != '\0') {
				hash = hash ^ str[index - 1];
				hash = hash * fnvPrime;
			}
			return hash;
		}

		static size_t combineHash(size_t seed, size_t hash) {
			return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
		}

	};
}