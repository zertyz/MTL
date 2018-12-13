#ifndef MUTUA_CPPUTILS_CONSTEXPRUTILS_H
#define MUTUA_CPPUTILS_CONSTEXPRUTILS_H

#include <sstream>

namespace Mutua::CppUtils {

	/** <pre>
	 * ConstExprUtils.h
	 * ================
	 * created by luiz, Dec 12, 2018
	 *
	 * Functions that returns their values in compile-time.
	 *
	 * The code here were taken/inspired from:
	 *   - https://mklimenko.github.io/english/2018/06/04/constexpr-random/
	 *   - 
	 *
	 * Another interesting lib: https://github.com/elbeno/constexpr
	*/
	struct ConstExprUtils {

		/** Converts the next two digits (at offset and offset+1) to a constexpr number */
		constexpr static auto substr2i(const char* str, int offset) {
			return static_cast<std::uint32_t>(str[offset] - '0') * 10 +
			static_cast<std::uint32_t>(str[offset + 1] - '0');
		}

		/** Returns a pseudorandom generator seed based on the compiler defined '__TIME__' */
		constexpr static auto getSeed() {
			auto t = __TIME__;
			return substr2i(t, 0) * 60 * 60 + substr2i(t, 3) * 60 + substr2i(t, 6);
		}

		/** Random number with the linear congruential generator, using the full 'uint32_t' range.
		  * With good parameters (modulus m, multiplier a and increment c) this algorithm provides a long series
		  * of non-repeating pseudo-random numbers (up to the 2^64) -- said the author.
		  * The 'previous' returned element must be passed, since constexprs cannot have static variables. */
		constexpr static std::uint32_t uint32Rand(std::uint32_t &previous) {
			previous = 1664525 * previous + 1013904223;
			return previous;
		}

		/** Same as #uint32Rand, but returns a range between 0 and 1.
		  * 'previous' must be passed between calls */
		constexpr static double doubleRand(std::uint32_t &previous) {
			auto dst = uint32Rand(previous);
			return static_cast<double>(dst) / 0xFFFFFFFF;
		}

		/** for a numeric '_ElementType', returns a 'std::array<_ElementType>' with '_Length' elements in the
		  * range ['min', 'max'] */
		template <typename _ElementType, std::size_t _Length>
		constexpr static auto generateRandomArray(_ElementType min, _ElementType max) {
			std::array<_ElementType, _Length> array{};
			auto previous = getSeed();
			for (auto &element : array) {
				element = static_cast<_ElementType>(doubleRand(previous) * ((max+1) - min) + min);
			}
			return array;
		}

	};

}
#endif //MUTUA_CPPUTILS_CONSTEXPRUTILS_H