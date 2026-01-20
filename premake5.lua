workspace "BorderlessFullscreen"
	configurations { "Release" }
	platforms { "Win32" }
	architecture "x32"
	characterset "MBCS"
	staticruntime "On"
	location "build"
	objdir "build/obj"
	buildlog "build/log/%{prj.name}.log"
	flags { "MultiProcessorCompile" }

	filter "configurations:Release"
		defines { "NDEBUG", "WIN32_LEAN_AND_MEAN" }
		optimize "Speed"
		linktimeoptimization "On"
		flags { "NoBufferSecurityCheck" }
		buildoptions { "/std:c++latest", "/O2", "/Os", "/GL", "/Gy", "/Gw", "/Zc:inline", "/GS-" }
		linkoptions { "/LTCG", "/OPT:REF", "/OPT:ICF", "/MERGE:.rdata=.text" }
		symbols "Off"

project "BorderlessFullscreen"
	kind "SharedLib"
	language "C++"
	targetdir "bin"
	targetextension ".asi"
	rtti "Off"
	exceptionhandling "Off"

	defines {
		"_CRT_SECURE_NO_WARNINGS",
		"rsc_CompanyName=\"CanerKaraca\"",
		"rsc_LegalCopyright=\"MIT License\"",
		"rsc_FileVersion=\"1.0.0.0\"",
		"rsc_ProductVersion=\"1.0.0.0\"",
		"rsc_InternalName=\"%{prj.name}\"",
		"rsc_ProductName=\"%{prj.name}\"",
		"rsc_OriginalFilename=\"%{prj.name}.asi\"",
		"rsc_FileDescription=\"https://github.com/CanerKaraca23/BorderlessFullscreen\"",
		"rsc_UpdateUrl=\"https://github.com/CanerKaraca23/BorderlessFullscreen\""
	}

	files {
		"source/**.cpp",
		"external/injector/safetyhook/include/**.hpp",
		"external/injector/safetyhook/src/**.cpp",
		"external/injector/zydis/**.h",
		"external/injector/zydis/**.c"
	}

	includedirs {
		"external/injector/include",
		"external/injector/safetyhook/include",
		"external/injector/zydis"
	}
