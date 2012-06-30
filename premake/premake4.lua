dofile "platforms.lua"
solution "tinyaudio_examples"
	
	local ROOT_DIR = path.join(path.getdirectory(_SCRIPT), ".." .. "/")

	configurations {
		"debug",
		"release",
	}

	platforms {
		"x32",
		"x64",
		"native",
	}

	language "C++"

	flags {
		"StaticRuntime",
		"NoMinimalRebuild",
		"NoPCH",
		"NoRTTI",
		"NoEditAndContinue",
		"ExtraWarnings",
		"FatalWarnings",
		"Symbols",
	}

	includedirs {
		ROOT_DIR .. "include/",
	}

	location (ROOT_DIR .. ".build/projects")
	objdir (ROOT_DIR .. ".build/obj/")
	targetdir (ROOT_DIR .. "bin/")

	configuration { "nacl32" }
		targetextension ".32.nexe"
		linkoptions { "-melf32_nacl" }
	configuration { "nacl64" }
		targetextension ".64.nexe"
		linkoptions { "-melf64_nacl" }
	configuration { "nacl*" }
		targetdir (ROOT_DIR .. "bin/nacl/")
		links {
			"ppapi",
		}


	project "sin"
		kind "ConsoleApp"

		files {
			ROOT_DIR .. "include/**.h",
			ROOT_DIR .. "examples/example_sin.cpp",
		}

		configuration { "windows" }
	
			includedirs {
				"$(DXSDK_DIR)/include",
			}

			files {
				ROOT_DIR .. "examples/main.cpp",
				ROOT_DIR .. "src/tinyaudio_xaudio.cpp",
			}

		configuration { "nacl*" }

			files {
				ROOT_DIR .. "examples/main_nacl.cpp",
				ROOT_DIR .. "src/tinyaudio_nacl.cpp",
			}

		configuration { "linux-alsa" }

			files {
				ROOT_DIR .. "examples/main.cpp",
				ROOT_DIR .. "src/tinyaudio_alsa.cpp",
			}

			links {
				"pthread",
				"asound",
			}

		configuration { "linux-pulse" }

			files {
				ROOT_DIR .. "examples/main.cpp",
				ROOT_DIR .. "src/tinyaudio_pulse.cpp",
			}

			links {
				"pthread",
				"pulse-simple",
			}
