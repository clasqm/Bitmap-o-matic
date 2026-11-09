An analysis of the visual interface of the historical **Icon-O-Matic** application (IconOMatic.png) reveals several core functional components and toolsets that a modern replacement must replicate to maintain true parity with legacy BeOS bitmap icon editing.

---

### 1. Menu Bar & Workspace Management

* **Standard Dropdowns:** Contains **File** (for loading/saving raw icon streams or attributes: Open, New, Save, Save As. Quit, ) and **Edit** options.  
* **Dual-Resolution State Switcher (Top Right Panel):**
* Displays thumbnail representations for both **Large Icons ($32 \times 32$)** and **Mini Icons ($16 \times 16$)**.
* Provides active selection states for both normal color and selected/disabled/alternative view variants, allowing seamless switching between editing the high-detail view and the tiny UI view.


### 2. Main Editing Canvas (Left Panel)

* **Pixel Grid Matrix:** A heavily gridded working area enforcing clear single-pixel boundaries ($32 \times 32$ layout shown).

* **Active Cursor Feedback:** Displays contextual tool cursors directly on the grid (such as the active **Pencil** tool shown with its modifier underscore).


### 3. Tool Palette (Middle Toolbar)

A dedicated row of raster editing tools, from left to right. These will also be presented in the "Edit" menu option.

1. **Selection Marquee:** Rectangular area selection for moving or editing blocks of pixels.
2. **Eraser:** Clears pixels back to transparent or background color.
3. **Pencil / Freehand Draw:** Primary freehand pixel plotting tool (currently active).
4. **Eyedropper / Color Picker:** Samples an existing color directly from the pixel grid.
5. **Paint Bucket / Flood Fill:** Fills contiguous areas of identical color indices.
6. **Line Tool:** Draws straight lines.
7. **Rectangle / Outline Box:** Draws unfilled rectangular shapes.
8. **Filled Rectangle:** Draws solid filled rectangular shapes.
9. **Rounded Rectangle (Outline):** Draws unfilled rectangles with curved corners.
10. **Filled Rounded Rectangle:** Draws solid rounded rectangles.
11. **Ellipse / Oval (Outline):** Draws unfilled circular/oval shapes.
12. **Filled Ellipse / Oval:** Draws solid filled circular/oval shapes.

### 4. Color Management & Palette (Bottom Panel)

* **Foreground / Background Swatches:** Displays active **Front** and **Back** color chips with an interactive swap/transparency toggle button (`T`) between them.

* **System Color Palette Grid:** A complete visual layout of the standard 256-color BeOS system color cube and ramp blocks.

* **RGB Numerical Readout/Adjustment:** Precision spin boxes or text fields for **Red**, **Green**, and **Blue** channel values (0–255 scale) corresponding to the selected palette index.


No mainstream, modern commercial bitmap editor (such as Photoshop, GIMP, or Aseprite) natively recognizes or supports the raw, uncompressed binary layout and proprietary 256-color BeOS system palette of classic BeOS bitmap icon attributes out-of-the-box. Because these icons are typically stored as BeFS file system attributes (BEOS:L:ICON) rather than standalone files with standard headers, general-purpose graphics software cannot natively open, parse, or write them without specialized custom plugins or scripts.However, you can easily inspect, modify, and manage them using alternative cross-platform approaches without ever booting an instance of BeOS:

1. Python Scripting / Custom Parsers (Any OS): Because the format is a straightforward, uncompressed raw pixel stream (1,024 bytes for $32 \times 32$, 256 bytes for $16 \times 16$) mapped to an 8-bit index table, a short Python script using libraries like Pillow can easily ingest these raw buffers, map them to an explicit BeOS 256-color palette, convert them to standard PNGs for editing in any modern tool, and pack them back into raw byte arrays.
2. Haiku OS Native Tools (Running on modern hardware/VM): Haiku (BeOS's open-source successor) includes native utilities and backward-compatibility layers that can read classic bitmap icon attributes, though Haiku heavily favors its own later vector format (BEOS:ICON).
3. Custom-Built Editor: Building a lightweight, dedicated cross-platform utility (using a framework like Qt, SDL, or a web canvas) is remarkably straightforward precisely because the format lacks complex headers, compression, or chunk structures. Your editor only needs to read/write fixed-size byte arrays coupled with a static 256-color lookup table.
