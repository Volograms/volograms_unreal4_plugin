// Copyright 2021 Volograms, Ltd.
// http://volograms.com/

using UnrealBuildTool;
using System.IO; // PATH etc.

public class volograms : ModuleRules
{
  private string UProjectPath
  {
    get { return Directory.GetParent(ModuleDirectory).Parent.FullName; }
  }

  public volograms(ReadOnlyTargetRules Target) : base(Target)
  {
    /* https://www.unrealengine.com/en-US/marketplace-guidelines#26
		2.6.3.c Plugins being built against engine versions 4.18 onward will be ensured to be IWYU compatible.
		Publishers can add the following to the plugin’s .build.cs files to enable IWYU: */
    PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

    // NOTE(anton) moved ThirdParty under the module dir so that plugin packager would include it
    string ThirdPartyPath = Path.GetFullPath(Path.Combine(UProjectPath, "Source/volograms/ThirdParty"));
    string FFMPEGPath = Path.Combine(ThirdPartyPath, "ffmpeg");
    string IncludePath = Path.Combine(FFMPEGPath, "include");
    PublicIncludePaths.Add(IncludePath);
    System.Console.WriteLine("VOL: IncludePath = " + IncludePath);

    if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
    {
      string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "Win32";
      string BinariesPath = Path.Combine(FFMPEGPath, "bin", "vs", PlatformString);
      string LibrariesPath = Path.Combine(FFMPEGPath, "lib", "vs", PlatformString);
      System.Console.WriteLine("VOL: LibrariesPath = " + LibrariesPath);
      System.Console.WriteLine("VOL: BinariesPath = " + BinariesPath);

      PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avcodec.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avdevice.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avfilter.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avformat.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "avutil.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "swresample.lib"));
      PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "swscale.lib"));

      string[] dlls = {
      "avcodec-59.dll",
      "avdevice-59.dll",
      "avfilter-8.dll",
      "avformat-59.dll",
      "avutil-57.dll",
      "swresample-4.dll",
      "swscale-6.dll"
      };


      // NOTE(Anton) this is a hack because I gave up trying to find out how to get U4 to properly link
      // the .dlls from within the plugin's binary directory. Absolute nightmare.
      string placetodumpdlls = Path.Combine(UProjectPath, "../../Binaries/", "Win64");

      foreach (string dll in dlls)
      {
        //CopyToBinaries(Path.Combine(BinariesPath, dll), Target);


        // copy dll to Binaries subfolder within plugin subfolder
        RuntimeDependencies.Add(Path.Combine(BinariesPath, dll), StagedFileType.NonUFS);

        // copy dll to game's Binaries subfolder because it wasnt looking in the above path to the plugin binaries subfolder
        string Destination = Path.Combine(placetodumpdlls, dll);
        string Source = Path.Combine(BinariesPath, dll);
        // this copies the .dll file from src to dst:
        RuntimeDependencies.Add(Destination, Source);


        PublicDelayLoadDLLs.Add(Destination);
      }
    }
    else if (Target.Platform == UnrealTargetPlatform.Mac)
    {
      //string LibrariesPath = Path.Combine(Path.Combine(ThirdPartyPath, "ffmpeg", "lib"), "osx");
      string LibrariesPath = "/usr/local/lib";
      System.Console.WriteLine("VOL: LibrariesPath = " + LibrariesPath);

      // TODO(Anton) consider using libraries that have the version number in the filename
      string[] libs = { "libavcodec.dylib", "libavdevice.dylib", "libavfilter.dylib", "libavformat.dylib", "libavutil.dylib", "libswresample.dylib", "libswscale.dylib" };
      foreach (string lib in libs)
      {

        // TODO(Anton) test and investigate how macos unreal plugins do this!!!
        // this is copy-paste from https://github.com/bakjos/FFMPEGMedia/blob/master/Source/FFMPEGMedia/FFMPEGMedia.Build.cs
        // seems _very_ bad-citizeny putting the dylibs directly in /usr/local/lib/
        // delayload seems like a better option IMO

        PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, lib));
        //PublicDelayLoadDLLs.Add(Path.Combine(LibrariesPath, lib));
        //CopyToBinaries(Path.Combine(LibrariesPath, lib), Target);
        //RuntimeDependencies.Add(Path.Combine(LibrariesPath, lib), StagedFileType.NonUFS);
      }

    } // TODO(Anton) other platforms (Android) here

    PublicIncludePaths.AddRange(
      new string[] {
				// ... add public include paths required here ...
			}
      );


    // NB(Anton) there are also Public include paths
    PrivateIncludePaths.AddRange(
      new string[] {
        "volograms/Private",
      }
      );

    PublicDependencyModuleNames.AddRange(
      new string[]
      {
        "Core",
        "ProceduralMeshComponent"
				// ... add other public dependencies that you statically link with here ...
			}
      );


    PrivateDependencyModuleNames.AddRange(
      new string[]
      {
        "CoreUObject",
        "Engine",
        "Slate",
        "SlateCore"
				// ... add private dependencies that you statically link with here ...	
			}
      );


    DynamicallyLoadedModuleNames.AddRange(
      new string[]
      {
				// ... add any modules that your module loads dynamically here ...
			}
      );
  }
}
