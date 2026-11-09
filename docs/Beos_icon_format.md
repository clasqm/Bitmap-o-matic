This formal description of the legacy **BeOS Bitmap Icon file and attribute format** (used natively in BeOS R3 through R5/BONE, distinct from Haiku’s later vector-based `BEOS:ICON` format) is structured to support the design and implementation of a dedicated bitmap icon editor.

---

# Formal Specification: Legacy BeOS Bitmap Icon Format

## 1. Overview & Context

In classic BeOS, icons are not typically stored as standalone single-file documents with a dedicated file extension on disk. Instead, they exist in two primary contexts:

1. **BeFS Extended Attributes:** Stored directly inside the file system metadata of files/directories under specific attribute names (`BEOS:L:ICON_TYPE` for large icons and `BEOS:M:ICON_TYPE` for mini icons, or generically referred to as attribute types `'ICON'` and `'MINI_ICON'`).
2. **Resource Files:** Embedded inside application resource forks or resource blocks using Be’s resource compiler (`rc`) format, under resource types `'ICON'` (Large, 32×32) and `'MINI_ICON'` (Mini, 16×16).

Unlike modern container formats that wrap images in headers, metadata tags, or compression schemes (such as PNG or ICO), classic BeOS bitmap icons are **raw, uncompressed pixel arrays mapped directly to a fixed 256-color system palette**.

---## 2. Geometric Dimensions & Specifications
A complete BeOS icon set for a single entity comprises two distinct fixed resolutions:

| Icon Type | Grid Dimensions | Total Pixels | Raw Data Size (Bytes) | BeFS Attribute Name / Resource Type |
| --- | --- | --- | --- | --- |
| **Large Icon** | $32 \times 32$ pixels | 1,024 pixels | **1,024 bytes** | `BEOS:L:ICON` / type `'ICON'` |
| **Mini Icon** | $16 \times 16$ pixels | 256 pixels | **256 bytes** | `BEOS:M:ICON` / type `'MINI_ICON'` |

* **Color Depth:** 8 bits per pixel (8 bpp).
* **Transparency:** 1-bit alpha channel handled via a designated transparent palette index.
* **Layout Structure:** Row-major order, top-to-bottom, left-to-right, with **no scanline padding** (each row is exactly equal to the width in pixels).

---## 3. The BeOS System Color Palette (256 Colors)
BeOS icons do not embed custom color palettes. Instead, every pixel byte ($0x00$ to $0xFF$) acts as an exact index into the **Standard BeOS 8-bit System Palette** (often called the *BeOS Netscape-safe/Cube-mixed Palette* or system color table).

For an editor to accurately render the icon, it must implement the canonical 256-color RGB table:

* **Indices $0x00$ through $0xFF$:** Pre-allocated combination of a uniform color cube, grayscale ramps, and system UI interface shades.
* **The Transparent Color Index:** Typically, index **`0x3F`** (or depending on specific system context, designated transparency values handled by the Tracker view drawing routines) represents transparency. *Editor design note:* Your editor should map a user-friendly checkerboard background to this specific transparent index and allow users to pick/modify palette colors cleanly.

---## 4. Binary Layout & Data Stream Specification

### 4.1 Large Icon (`32x32`)

* **Total Size:** Exactly 1,024 bytes.
* **Structure:**

$$\text{File/Attribute Stream} = \left[ P_{0,0}, P_{0,1}, \dots, P_{0,31}, P_{1,0}, \dots, P_{31,31} \right]$$


* Each byte represents one pixel coordinate $(x, y)$ where $x \in [0, 31]$ and $y \in [0, 31]$.
* Byte offset for pixel at column $x$ and row $y$:

$$\text{Offset} = (y \times 32) + x$$





### 4.2 Mini Icon (`16x16`)

* **Total Size:** Exactly 256 bytes.
* **Structure:**
* Each byte represents one pixel coordinate $(x, y)$ where $x \in [0, 15]$ and $y \in [0, 15]$.
* Byte offset for pixel at column $x$ and row $y$:

$$\text{Offset} = (y \times 16) + x$$





---## 5. File & Attribute Serialization Requirements for an Editor

To make an editor practical for modern host systems (like Linux, macOS, or Windows) while fully supporting legacy BeOS compatibility, the editor's file architecture should support:

1. **Native Raw Binary Export/Import (.raw / .icon):**
* Ability to read and write raw binary dumps of exactly 1,024 bytes (for 32×32) or 256 bytes (for 16×16).


2. **BeFS Attribute Container Mapping:**
* Provide an ingestion/export filter that can inject these byte streams directly into a file's extended attributes (simulating BeFS attribute streams for cross-compilation or disk image crafting).


3. **Multi-Icon Workspace Model:**
* A project file (e.g., `.beicon` or a native JSON/XML wrapper) that binds both the Large ($32 \times 32$) and Mini ($16 \times 16$) canvases into a single editing session, since standard BeOS items require both to display correctly in Tracker's *Icon View* and *Mini Icon View*.



---## 6. Suggested Architectural Guidelines for the Editor

* **Canvas Grid Component:** A discrete grid matrix ($32 \times 32$ cells and $16 \times 16$ cells) with snapping, pencil, bucket fill, eyedropper, and selection marquee tools.
* **Palette Swatch Panel:** A fixed UI panel displaying the 256 BeOS system color blocks, complete with a designated "Transparent" swatch indicator.
* **Real-time Preview Widget:** A secondary preview pane scaling the raw pixel buffers at 1:1 scale next to a Tracker-style file listing mockup to inspect legibility at native OS resolutions.
