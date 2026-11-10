# Bitmap-o-matic
A program to edit old BeOS bitmap icons

Goal: Recreate the old BeOS Icon-o-matic, a clean room implementation that does not have access to BeOS code.

Justification: Twenty years after the introduction of HVIF icons, the old BeOS bitmap icons still work. But the ability to edit those icons was taken away. This project restores it.

Projection: It is not expected that there will be a renaissance in the creation of bitmap icons. HVIF does have genuine advantages. However, it is difficult to work with and only two or three people have really mastered it, as opposed to the dozens of iconsets that were produced by various people in the BeOS era. At the very least, we should be able to edit the old icons.

Bitmap-o-matic is a native Haiku application that edits the 32 × 32 and
16 × 16 B_CMAP8 images BeOS stored in a file's extended attributes, rather
than the vector HVIF icons used by Haiku today.

## Features

* Opens and saves:

  * Bitmap-o-matic project files (`.beicon`)
  * Raw BeOS icon streams: 1024 bytes (32 × 32) and 256 bytes (16 × 16)
  * Files that carry `BEOS:L:ICON` / `BEOS:M:ICON` attributes (also the
    `BEOS:L:STD_ICON` / `BEOS:M:STD_ICON` names used by Haiku's Tracker)

* Edits the large (32 × 32) and mini (16 × 16) images of an icon set. The
  active image is chosen with the preview thumbnails or the *Edit → Icon*
  menu. The selected-state variants the file format carries are written as
  copies of the two edited images.

* Twelve tools: selection, eraser, pencil, eyedropper, bucket fill, line,
  rectangle, filled rectangle, rounded rectangle, filled rounded rectangle,
  ellipse and filled ellipse.

* The 256-colour BeOS/Haiku system palette, a front and a back colour chip,
  an RGB entry field and a *T* button that makes the back colour transparent.
  Hovering over a tool button shows its name.
  Left-drag draws with the front colour, right-drag with the back colour.

* A grid canvas with a checkerboard behind transparent pixels, marching-ants
  selection, moving and pasting of selections, and undo/redo.

* *Edit → Select All Transparent* and *Select All Non-transparent* select
  exactly the transparent or the drawn pixels, not a rectangle.

* A *Help* menu with an About box and a help window.

## Building

Bitmap-o-matic uses the Haiku API, linking against `libbe` and `libtracker`
(the file panels live in the latter). It has no other dependencies.

```sh
mkdir build
cd build
cmake ..
make
```

The executable is `build/Bitmap-o-matic`. `make clean` works as usual; no
installation step is needed, the binary can be run straight from the build
directory.

## Using it

Start the program and use *File → Open…* to load an icon, or draw a new one
and save it.

*Save* and *Save As…* write a Bitmap-o-matic project file and then attach the
bitmap icons to that same file, so Tracker shows the icon straight away.
*File → Attach Icons to File…* writes the icons into the attributes of a
different file that already exists. Older 1024- and 256-byte B_CMAP8 icon
streams can still be opened, and a file opened that way is saved back in the
same form.

## Notes

* Transparency is palette index `0xFF` (`B_TRANSPARENT_MAGIC_CMAP8`), the
  last entry of the system palette. Older documentation sometimes mentions
  index `0x3F`; that value is an ordinary colour in the system palette and is
  not treated as transparent here.
* HVIF icons are not read or written. This program is deliberately limited to
  the legacy bitmap formats.
* Project files start with the magic `BEOSICON` followed by a version and the
  four images, stored little-endian.
