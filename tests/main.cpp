#include <X97.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Testing septet") {
	std::uint8_t input = 126;
	std::uint8_t theirs = input;
	std::uint8_t ours = X97::septet(input);

	REQUIRE(theirs == ours);
}
