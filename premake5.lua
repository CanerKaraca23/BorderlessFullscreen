
function setupDebugger(gameAbbr, gameExeName)
    postbuildcommands { "\
if defined GTA_" .. gameAbbr .. "_DIR ( \r\n\
taskkill /IM " .. gameExeName .. " /F /FI \"STATUS eq RUNNING\" \r\n\
xcopy /Y \"$(TargetPath)\" \"$(GTA_" .. gameAbbr .. "_DIR)\\scripts\" \r\n\
)" }

    debugcommand ("$(GTA_" .. gameAbbr .. "_DIR)\\" .. gameExeName)
    debugdir ("$(GTA_" .. gameAbbr .. "_DIR)")
end

workspace "SA.WindowedMode"
   configurations { "Release", "GtaSA" }
   platforms { "Win32" }
   architecture "x32"
   characterset ("MBCS")
   staticruntime "on"
   location "build"
   objdir ("build/obj")
   buildlog ("build/log/%{prj.name}.log")
   buildoptions {"-std:c++latest"}
      
project "SA.WindowedMode"
   kind "SharedLib"
   language "C++"
   targetdir "data/%{cfg.buildcfg}"
   targetextension ".asi"
   
   defines { "rsc_CompanyName=\"ThirteenAG\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   defines { "rsc_FileVersion=\"1.0.0.0\"", "rsc_ProductVersion=\"1.0.0.0\"" }
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.asi\"" }
   defines { "rsc_FileDescription=\"https://github.com/ThirteenAG\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/III.VC.SA.WindowedMode\"" }
   
   files { "source/*.h" }
   files { "source/*.cpp", "source/*.c" }
   files { "source/*.rc" }
   files { "external/injector/safetyhook/include/**.hpp", "external/injector/safetyhook/src/**.cpp" }
   files { "external/injector/zydis/**.h", "external/injector/zydis/**.c" }

   includedirs { "source" }
   includedirs { "source/d3d8" }
   includedirs { "external/injector/safetyhook/include" }
   includedirs { "external/injector/zydis" }
   includedirs { "external/injector/include" }
   includedirs { "external/IniReader" }

   filter "configurations:GtaSA"
      defines { "DEBUG" }
      symbols "on"
      setupDebugger("SA", "gta_sa.exe")

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "on"
      targetdir "data"
