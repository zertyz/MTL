#ifndef MTL_COMPILETIME_ConstExprUtils_hpp
#define MTL_COMPILETIME_ConstExprUtils_hpp

#include <sstream>

namespace mtl::compiletime {

	/** <pre>
	 * ConstExprUtils.hpp
	 * ==================
	 * created by luiz, Dec 12, 2018
	 *
	 * Some possibly-useful functions that returns their values in compile-time.
	 *
	 * Functionalities:
	 *   1) Random numbers, arrays & strings
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
		constexpr static std::uint32_t getSeed() {
			const char* t = __TIME__;
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
			return static_cast<double>(uint32Rand(previous)) / 0xFFFFFFFF;
		}

		/** for a numeric '_ElementType', returns a 'std::array<_ElementType>' with '_Length' elements in the
		  * range ['min', 'max']
		  * Please note that the array should be declared as constexpr or else it will not be rendered at compile time. Example:
		  * constexpr array<char, 17*1024'000> constexprHugeArray = Mutua::CppUtils::ConstExprUtils::generateRandomArray<char, 17*1024'000>('a', 'z'); */
		template <typename _ElementType, std::size_t _Length>
		constexpr static std::array<_ElementType, _Length> generateRandomArray(_ElementType min, _ElementType max) {
			std::array<_ElementType, _Length> array{0};
			auto previous = getSeed();
			for (auto &element : array) {
				element = static_cast<_ElementType>(doubleRand(previous) * ((max+1) - min) + min);
			}
			return array;
		}

		/** Generate a pseudo 2D char array suitable to be used in generating a string array by #generateStringArrayFromCharArray.
		 *  Note that _2DArrayLength must take into account the leading '\0', so the length of the '_1DArrayLength' strings will be _2DArrayLength-1.
		 *  Usage example:
		 *  constexpr std::array<std::array<char, 17>, 1024'000> constexprRandom2DCharArray = Mutua::CppUtils::ConstExprUtils::generateRandomChar2DArray<1024'000, 17>('a', 'z');
		 *  constexpr array<string_view, 1024'000>               constexprRandomStringArray = Mutua::CppUtils::ConstExprUtils::generateStringArrayFromCharArray<1024'000, 17>(constexprRandom2DCharArray); */
		template <unsigned _1DArrayLength, unsigned _2DArrayLength>
		constexpr static std::array<std::array<char, _2DArrayLength>, _1DArrayLength> generateRandomChar2DArray(char min, char max) {
			std::array<std::array<char, _2DArrayLength>, _1DArrayLength> array{''};
			std::uint32_t previous = getSeed();
			for (auto &charArrayElement : array) {
				unsigned i = 0;
				for (auto &character: charArrayElement) {
					if (++i == _2DArrayLength) {
						character = '\0';
					} else {
						character = static_cast<char>(doubleRand(previous) * ((max+1) - min) + min);
					}
				}
			}
			return array;
		}

		/** Used in conjunction with #generateRandomChar2DArray() to generate a random string array. */
		template <unsigned _1DArrayLength, unsigned _2DArrayLength>
		constexpr static std::array<std::string_view, _1DArrayLength> generateStringArrayFromCharArray(const std::array<std::array<char, _2DArrayLength>, _1DArrayLength>& char2DArray) {
			std::array<std::string_view, _1DArrayLength> array{0};
			unsigned i = 0;
			for (auto &stringElement : array) {
				stringElement = std::string_view(char2DArray[i++].data());
			}
			return array;
		}

	};

}
#endif //MTL_COMPILETIME_ConstExprUtils_hpp
