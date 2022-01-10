# Volograms Unreal 4 Plugin #

* Current version:              0.6.1 (beta)
* Built against Unreal version: 4.27

![Volograms are animated 3D volumetric captures.](rafafloss.gif "Volograms are animated 3D volumetric captures.")

### Contents and Features of the Plugin ###

* Drag-and-drop VologramActor class Blueprint integration for use in the Editor.
* VologramMaterial class for the video texture (used automatically by the Actor class).
* C++ access to key functionality from the Volograms SDK if you prefer to customise your own vologram player.
* Unreal users can load and play a Vologram without writing any code.

### Operating Systems Supported ###

* Only 64-bit Windows systems are supported currently.
* MacOS and iOS are work-in-progress.
* Android is planned for a future version.
* 32-bit Windows can be added if required.

### How do I get set up? ###

* If you don't have an existing Unreal project then create a new game project.
* Download as an archive and unzip into your `<Game>/Plugins/` directory (if this is your first plugin in your game you may have to create the `Plugins` directory).
* Relaunch your game project in the Unreal Editor, and in the *Plugins* menu in Unreal Editor enable the *Volograms* plugin.
* From the *Content browser* (at the bottom of Unreal Editor).
    * Go to the *View Options* drop-down in the corner, and make sure *Show Plugin Content* is selected.
    * Find the `VologramActor` Blueprint, and right click on it to 'Create Child Blueprint Class'.
* Drag your new vologram actor blueprint into your scene.
* In the side context panel that appears when your vologram is selected:
    * Set the paths to the vologram *header* .vols file,
    * the *sequence* .vols file,
    * and the *video texture* file. 
    * Ensure that *VologramMaterial* is selected in the *Material* drop-down.
* Hit *play* to preview your vologram in Unreal.

![The end product should display like this.](antonvologram.png "The end product should display like this!")

### Packaging Your Project

* Double-check in the *VologramActor* that the paths to your vologram files will be the correct relative or absolute paths when packaged.
* Because the package will include the Volograms plugin, Unreal requires you to compile your project.
    * If you have a Blueprint project you will need to convert it to C++ first. You can do this by creating a new, blank, C++ file, which will generate a Visual Studio project that you can compile from.

### Adding Audio to the Vologram

* If you have an audio file i.e. a `.wav`, you can drag this into your Unreal Editor content panel to use as an audio source.
* (Optional) Edit your audio source properties to add a 3D attenuation so that the volume decreases as the camera moves away from the vologram.
* Edit your `VologramActor` blueprint Event Graph, and attach a *Play Sound at Location* node to the *BeginPlay* event. Choose your audio source in the drop-down menu.

![Adding our audio source to play to the BeginPlay event.](adding_sound_file.png "Playing our sound with the VologramActor")

### Dependencies and Licence ###

* Source code included in this plugin copyright 2021 Volograms is provided under the terms of the MIT licence (see LICENSE.md).
* Other terms apply to third-party code found in the `Source/ThirdParty` sub-directory:
    * This plugin depends on [ffmpeg](https://www.ffmpeg.org/) for video playback, which is included in the plugin. This is used under the LGPL licence.
    * The source code for ffmpeg, and various notices, must be attached to any distribution of this plugin. See the ffmpeg website's Legal page for further details.
    * The MPEG-4 and h264 standards are used by this plugin, which may not be suitable for some commercial uses.

### Known Issues ###

* Older Volograms may have inconsistent orientation and scale, and earlier versions will be missing normals. If you need to load these let us know.
* There is no built-in demo or functions for seek/rewind/loop yet. We are happy to take feature requests based on early-adopter testing.

### Future Plans and Ideas ###

* Simpler file loading.
* A video tutorial explaining how to use this plugin.
* Audio playback using any audio track found in the vologram video file.
* User interface style controls to rewind/seek/loop/pause/play. Probably a library of Blueprint functions would be the most idiomatic approach here, but we could also tie these to a GUI panel in a demo application. 
* For editor integration an Unreal 'Factory' class could be implemented. This would provide a context menu ability to create 'Volograms' assets, rather than manually creating a Blueprint from our custom Actor class. 

### Unity and OpenGL/DirectX Support ###

* We also have an equivalent Unity plugin using the same base libraries.
* The core libraries are implemented in C, and can be compiled into .dlls.
* These are `vol_geom`, which processes vologram header and sequence files into geometry, and `vol_av`, which is a wrapper over ffmpeg.
* To use ffmpeg for texture playback in another project, the `vol_av` code can be compiled into a `.dll` as a Unity plugin, with a little bit of project-specific hook-up code added.
* To play a vologram in OpenGL or DirectX, the `vol_geom` and `vol_av` files can be dropped into a program's source build, and the functions accessed directly from your program.
