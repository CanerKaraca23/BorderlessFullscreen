
function setupDebugger(gameAbbr, gameExeName)
    postbuildcommands { "\
if defined GTA_" .. gameAbbr .. "_DIR ( \r\n\
taskkill /IM " .. gameExeName .. " /F /FI \"STATUS eq RUNNING\" \r\n\
xcopy /Y \"$(TargetPath)\" \"$(GTA_" .. gameAbbr .. "_DIR)\\scripts\" \r\n\
)" }

    debugcommand ("$(GTA_" .. gameAbbr .. "_DIR)\\" .. gameExeName)
    debugdir ("$(GTA_" .. gameAbbr .. "_DIR)")
end

workspace "BorderlessFullscreen"
   configurations { "Release", "GtaSA" }
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
   
   defines { "WIN32_LEAN_AND_MEAN", "VC_EXTRALEAN", "NOMINMAX" }  -- Minimize Windows headers
   defines { "rsc_CompanyName=\"ThirteenAG\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   defines { "rsc_FileVersion=\"1.0.0.0\"", "rsc_ProductVersion=\"1.0.0.0\"" }
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.asi\"" }
   defines { "rsc_FileDescription=\"https://github.com/ThirteenAG\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/III.VC.SA.WindowedMode\"" }
   
   files { "source/*.cpp" }

   includedirs { "external/injector/include" }

   filter "configurations:GtaSA"
      defines { "DEBUG" }
      symbols "on"
      setupDebugger("SA", "gta_sa.exe")

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "Speed"
      flags { "LinkTimeOptimization", "NoBufferSecurityCheck" }
      buildoptions { "/Gy", "/Gw", "/GL" }  -- Function-level linking, Whole program optimization, Global data optimization
      linkoptions { "/LTCG", "/OPT:REF", "/OPT:ICF", "/MERGE:.rdata=.text" }  -- Link-time code gen, Remove unused, Fold identical, Merge sections
      symbols "Off"  -- Strip debug symbols
      targetdir "bin"
