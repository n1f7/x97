#include <array>
#include <sstream>

#include <X97.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Testing septet") {
	REQUIRE(std::uint8_t{126} == X97::septet(std::uint8_t{126}));
	REQUIRE(std::uint16_t{0x424} == X97::septet(std::uint16_t{0x224}));
}

TEST_CASE("Testing simple packet") {
	X97::Packet pack(X97::Command::SetRegsRpl);
	std::array<std::uint16_t, 2> args{0x224, 0x14};
	pack.appendArguments(std::begin(args), std::end(args));

	std::stringstream ss;
	ss << pack;

	const std::string theirs = "95 03 03 0B 27\n24 04 14 00 \n44 2A";

	REQUIRE(theirs == ss.str());
}
