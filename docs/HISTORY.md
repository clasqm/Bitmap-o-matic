# Bitmap-o-matic

**Goal:** Recreate the old BeOS Icon-o-matic, a clean room implementation that does not have access to BeOS code. Working title: Bitmap-o-Matic

**Justification:** Twenty years after the introduction of HVIF icons, the old BeOS bitmap icons still work. But the ability to edit those icons was taken away. This project restores it.

**Projection:** It is not expected that there will be a renaissance in the creation of bitmap icons. HVIF does have genuine advantages. However, it is difficult to work with and only two or three people have really mastered it, as opposed to the dozens of iconsets that were produced by various people in the BeOS era. At the very least, we should be able to edit the old icons.

**Preparation:** Use Gemini to document the BeOS bitmap icon format and to analyze an image of the old BeOS Icon-o-Matic. These documents will later be included in the program documentation. Also using my regular AGENTS.md

**Workflow:* I am running Freebuff in a browser, not on the CLI. In fact, I often run it on a non-Haiku computer. The LLM is therefore instructed not to try and compile, since it doesn't have a Haiku setup on its side anyway. When it is done it saves directly to a github repo. I then download a copy and attempt to compile it and report back to the LLM. I may change this workflow at some stage, but for now it is fine for my humble requirements.

## My prompts to FreeBuff

**MC: Create a bitmap editor that can open, edit and save old-style BeOS bitmap icons (NOT Haiku HVIF icons). It does not have to be visually identical to BeOS Icon-o-matic but must offer equivalent functionality.**

\<Observe Freebuff preparing to create a QT5 application. Stop execution\>

**MC: This is not a qt5 conversion. Use C++ and the haiku API. Continue.**

*Freebuff: Cloud runner restarted. Completed steps are saved; send a message to continue.*

**MC: continue**

\<3 hours later\>

*Freebuff: The application is now complete: sources, build file and documentation are all in the worktree.

Nothing has been committed or pushed; the new files are untracked/modified in the working tree.*

**MC: commit and push to main branch**

\<Two rounds of attempting to compile and returning the error codes to FreeBuff. Compiling on the thrird attempt. Testing the app. It works but needs fine-tuning\> 

**MC: Make the following refinements:

+ Add tooltips to the tool buttons
+ Remove the "Large selected" and "Mini selected" versions from the menu and interface. Haiku is not displaying those. If the file format demands it, just make them the same as the regular Large and mini versions upon saving.
+ That leaves the Large and Mini thumbnail depictions. Move Mini below Large.
+ Remove the UI element with the index number in the bottom right corner. Move the foreground and background colour widget to this location, then move the all the widgets on this level to the left. 
+ Resize the entire UI accordingly to make it narrower and minimize unused space.
+ Make the application window non-zoomable and non-resizable.
+ On the UI element at the very bottom, remove the duplication of information: either "(16 x 16)" or " --- 16 by 16 pixels" but not both. Once a file is loaded or a new file has been saved, append the full pathname in this space.
+ When saving a file, automatically also attach the icon to the resulting file.
+ Application does not exit cleanly. Please fix.
+ in the Edit menu, add new Item: Select All Transparent
+ in the Edit menu, add new Item: Select All Non-transparent
+ Create the functionality for these two new items.
+ Remove Export as Raw facility for both sizes.
+ Create a Help menu item with items About and Help
	+ About - dislay an Alert with the name of the program, copyright notice in the name of Michel Clasquin-Johnson, acknowledgement that Gemini and Freebuff were used in the construction of this application, and brief Licensing information. 
	+ Help - Write text-only help content describing the goal and basic functionality of the program. Display this help content in a window.**

\< one hour later\>

*Freebuff: Everything is implemented and pushed to main.*

**MC: We're getting there. Observe the attached screenshot. The first row of widgets is OK. In the second row, the buttons have become very small and some are completely offscreen. In the third row,  align the top of the Front/Back selector with the top of the RGB selector.**

**MC: Perfect! One last thing: write a report of about 500 words in which you document the steps taken to create this application. Just print it onscreen: I will cut and paste to a file on this side.**


## Building Bitmap-o-matic

### Starting point. 

The aim was a native icon editor for Haiku that works with the old BeOS bitmap icon format — the 32 × 32 and 16 × 16 B_CMAP8 images stored in BEOS:L:ICON and BEOS:M:ICON file attributes. The constraints were fixed from the start: C++ against the Haiku kits only, no Qt, no HVIF vector icons, and a plain CMake build that must work with mkdir build && cd build && cmake .. && make, with no install step.

### Architecture. 

The program was split into small pieces with one job each. IconDocument owns the four image slots the file format defines, the palette indices, the dirty flag and the file I/O — reading and writing both the attribute form and a project file, plus a raw 1024/256-byte stream for interoperability with older BeOS editors. IconCanvasView draws the pixel grid over a checkerboard, handles the pencil, eraser, fill, line, rectangle, ellipse, picker and marquee tools, and keeps an undo stack. Palette, Panels and EditorWindow provide the 256-colour grid, the previews, the tool bar and the menus. A Messages.h header collects the message constants so the window and its child views never include each other unnecessarily.

### First build. 

The first compile exposed several API mistakes: the code had used BView::SetFrame(), which sits in the private part of the Haiku API, so it was replaced with a helper that calls MoveTo() and ResizeTo(). BAlert was being constructed with the wrong argument order, several BView constructors were missing their resize mode, and BFilePanel turned out to live in libtracker, not libbe — so the link line grew to be tracker. After those fixes the program built and ran.

### Interface pass. 

A long list of refinements followed. Each tool button became its own small view, because a tool tip belongs to a whole view rather than a region, so twelve child views now carry one tool tip each. The two "selected" image variants were removed from the interface — the format still stores them, so saving mirrors the edited images into those slots. The preview became a vertical stack with Large above Mini. The window was narrowed, made non-resizable and non-zoomable, and the bottom row was rebuilt so the palette, RGB fields and colour chips share it without dead space. The status line stopped repeating the icon size and now appends the full path. Saving attaches the icons to the file it just wrote. Two new commands select all transparent or all non-transparent pixels as a real mask rather than a rectangle, the raw export commands went away, and a Help menu gained an About box and a scrollable help window.

### The stubborn tool bar. 

After the layout change the tool buttons came out tiny and half of them hung outside the window. The cause was a Haiku rule: FrameResized() is only delivered to views carrying the B_FRAME_EVENTS flag. The bar had laid out its buttons once, in its constructor, while it was still zero pixels wide. Adding the flag — and guarding the size calculation against a zero width — fixed it, and the chips were top-aligned with the RGB fields at the same time.

### Verification and limits. 

There is no Haiku toolchain in the development environment, so every change was checked with g++ -fsyntax-only -Wall -Wextra against the real Haiku headers cloned from the Haiku repository, with type shims for the compiler-specific integer typedefs. That catches signature and API errors but proves nothing about linking or appearance; the manual builds and screenshots came from the Haiku machine, and each round of feedback was folded back in. The result is a small, self-contained editor that opens, edits and saves legacy BeOS icons and displays them in Tracker.
