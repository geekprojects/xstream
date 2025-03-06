XStream
==

XStream uses the GStreamer library to stream parts of X-Plane textures via an embedded
H264 RTSP server. This means you can display PFDs, ECAMs etc directly from your sim to
another screen.

## Notes
This project was inspired by [XTextureExtractor](https://github.com/waynepiekarski/XTextureExtractor)
but uses a standard H264 RTSP stream and doesn't require a specific app or program to
use. (And I couldn't get it working on my Mac!)

This is a work in progress, it currently works with the ToLiss A321's PFD, ND and ECAM.
But I am working on adding configuration files. There is also a couple of seconds delay
that I'm trying to reduce! And the impact on FPS appears to be minimal for me, at least!


# Usage

Just compile and put the library in to your Resources/plugins directory as xstream.xpl

Then fire up your aircraft, start the stream from the menu and connect VLC/mpv etc to:
rtsp://&lt;ip-address&gt;:8554/pfd (or /nd or /ecam).


## Required Libraries
* yaml-cpp
* GStreamer

It currently requires my [UFC](https://github.com/geekprojects/ufc) library, but I'm
planning on removing that dependency.


# Contributions
Contributions of code and display definitions are more than welcome! Send me a PR!


# License
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
