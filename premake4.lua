solution("CryGame")
	location("solutions/vcproj" .. _ACTION)
	platforms { "x32", "x64" }

	language("C++")

	configurations{"Debug", "Release"}

	project("jio")
		location("solutions/" .. _ACTION)

		flags("NoMinimalRebuild")

		buildoptions
		{
			"/Gy",
			"/GS",
			"/GF",
			"/MP",
			"/arch:SSE",
		}

		language("C++")
		kind("Exe")

		targetdir("game/bin32/")
		targetname("CryGameSDK")

		objdir("obj/")

		includedirs(".")
		includedirs("external/protobuf-2.6.0/src")
		includedirs("test")
		

		files("**.hpp")

		configuration("Debug")
			flags("Symbols")

		configuration("Release")
			flags("Optimize")