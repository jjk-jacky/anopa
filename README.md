
# anopa: init system/service manager build around s6 supervision suite

anopa is an collection of tools and scripts aimed to provide an init system and
service manager for Linux systems, based around the s6 supervision suite[1].

It provides some execline[2] scripts that can be used as init for different
stage of the boot process, leaving stage 2 to be handled by s6-svscan, as well
as tools that can be used to create a runtime repository of servicedirs,
start/stop them and other related functions.

## Free Software

anopa - Copyright (C) 2015-2016 Olivier Brunel <jjk@jjacky.com>

anopa is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

anopa is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
anopa (COPYING). If not, see http://www.gnu.org/licenses/

## Want to know more?

Some useful links if you're looking for more info:

- [official site](http://jjacky.com/anopa "anopa @ jjacky.com")

- [source code & issue tracker](https://github.com/jjk-jacky/anopa "anopa @ GitHub.com")

- [origin story](http://jjacky.com/2015-04-10-has-arch-lost-its-way "Has Arch lost its Way? @ jjacky.com")

- [PKGBUILD in AUR](https://aur.archlinux.org/packages/anopa "AUR: anopa")

- [1: s6 supervision suite](http://skarnet.org/software/s6/ "s6 @ skarnet.org")

- [2: execline](http://skarnet.org/software/execline/ "execline @ skarnet.org")

Plus, anopa comes with man pages.
