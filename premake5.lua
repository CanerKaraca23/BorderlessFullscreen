workspace "BorderlessFullscreen"
   configurations { "Release" }
   platforms { "Win32" }
   architecture "x32"
   characterset ("MBCS")
   staticruntime "on"
   location "build"
   objdir ("build/obj")
   buildlog ("build/log/%{prj.name}.log")
   buildoptions {"-std:c++latest"}
   flags { "MultiProcessorCompile" }
      
project "BorderlessFullscreen"
   kind "SharedLib"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"
   targetextension ".asi"
   rtti "Off"  -- Disable RTTI
   exceptionhandling "Off"  -- Disable exceptions
   
   defines { "WIN32_LEAN_AND_MEAN", "VC_EXTRALEAN" }
   defines { "_CRT_SECURE_NO_WARNINGS" }
   defines { "rsc_CompanyName=\"CanerKaraca\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""}
   defines { "rsc_FileVersion=\"1.0.0.0\"", "rsc_ProductVersion=\"1.0.0.0\"" }
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.asi\"" }
   defines { "rsc_FileDescription=\"https://github.com/CanerKaraca23/BorderlessFullscreen\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/CanerKaraca23/BorderlessFullscreen\"" }
   
   files { "source/*.cpp" }
   files { "external/injector/safetyhook/include/**.hpp", "external/injector/safetyhook/src/**.cpp" }
   files { "external/injector/zydis/**.h", "external/injector/zydis/**.c" }

   includedirs { "external/injector/include" }
   includedirs { "external/injector/safetyhook/include" }
   includedirs { "external/injector/zydis" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "Speed"
      linktimeoptimization "On"
      flags { "NoBufferSecurityCheck" }
      buildoptions { "/O2", "/Os", "/GL", "/Gy", "/Gw", "/Zc:inline", "/GS-" }
      linkoptions { "/LTCG", "/OPT:REF", "/OPT:ICF", "/MERGE:.rdata=.text", "/IGNORE:4075" }
      symbols "Off"
      targetdir "bin"
