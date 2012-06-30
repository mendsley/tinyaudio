tinyaudio
=========
tinyaudio provides a cross-platform interface for sending 16-bit stereo PCM
data to the device's audio hardware.

tinyaudio does NOT provide any type of mixing (gain, voice, panning,
playback) support. I do provide another library (tinymixer) that
provides a "normal" audio interface for a game project. The two projects
are meant to work together; For sanity I keep tinymixer free from all
platform specific code.

Current support exists for:
- Windows (xaudio)
- Chrome NativeClient
- null driver
- Linux (ALSA)
- Linux (pulse)

Contact
-------
[@MatthewEndsley](https://twitter.com/#!/MatthewEndsley)  
<https://github.com/mendsley/tinyaudio>

License
-------
Copyright 2012 Matthew Endsley

This project is governed by the BSD 2-clause license. For details see the file
titled LICENSE in the project root folder.

Integrating with your codebase
------------------------------
The easiest way to integrate tinyaudio into your project is to simply include
the appropriate .cpp file inside of /src/ directly in your project.

Alternatively you can compile your platform's driver .cpp into a static
library and link it.

For AirMech I literally have the following code in sound/compile/device.cpp:

    #if HEAT_COMPILER_MSVC
    #	include <tinyaudio_xaudio.cpp>
    #elif HEAT_PLATFORM_NACL
    #	include <tinyaudio_nacl.cpp>
    ...
    #else
    #	include <tinyaudio_null.cpp>
    #endif

Including a .cpp file directly may feel dirty, but it's a heck of a lot easier
than managing _another_ 3rdparty pre-built library.


How to use tinyaudio in your game
---------------------------------

Simply call `tinyaudio::init` with a callback handler to generate 16bit stereo
PCM data. tinyaudio will invoke your callback when data is needed by the
hardware. Simply fill the buffer in a timely manner to avoid audio lag.

Platform drivers
----------------
`src/tinyaudio_alsa.cpp` ALSA support for Linux  
`src/tinyaudio_nacl.cpp` Support for 32/64bit NativeClient applications  
`src/tinyaudio_null.cpp` Null implementation of the interface  
`src/tinyaudio_pulse.cpp` Pulse audio support for Linux  
`src/tinyaudio_xuadio.cpp` Support for XAudio2 on Windows or XBox360  
