0.1
---
- well

0.2
---
- added glowicons

0.3
---
- added threading system
- added directory navigation

0.4
---
- removed flickering problem

0.5
---
- fixed crash bug, improved clipping

0.6
---
- added icon cache
- directory scanning can now be aborted
- hopefully fixed the Henes-o-crash
- added menus, 'Quit' is the only one which currently does something :)

0.7
---
- hopefully fixed the Henes-o-crash (again)
- added datatypes support
- added background support. Hardcoded to use ram:image1 and ram:image2
  currently

0.8
---
- added remapping for MagicWBv1 and MagicWB 16-colors (and fixed the original
  one too)
- cached icons reappear unselected
- fixed icon display trashing
- added handling for icons with one image only, in that case the 2nd image is
  the same as the 1st
- added prefs support, the background can be changed. Currently saved to
  PROGDIR:Ambient.prefs

0.9
---
- added more sanity checks for people using broken icons on their devices (hi
  Laire :) The error is shown on the bubble help over the "phantom" icon
- better icon text positioning (centered, proper clipping, etc..)
- icon help will contain useful infos everytime now (MUI bubble help)
- added sanity checks for newicons
- added sanity checks for normal images

0.10
----
- added color and font support for the icons
- added font styles (bold, italic, inversed)
- added background color for icon text
- added configurable spacing between the text and the icon

0.11
----
- added a 'Save' button into the prefs
- rewrote a lot of the prefs handling even if no one will notice :)
- non-transparent newicons are really not transparent anymore
- no more weird overlapped icons into windows
- added a transparent logo in the about screen. Needs today's MUI build
- added selective debugging. Use the Debug menu to switch subsystems on/off.
  Their state is saved with the config file
- Phantom Icons (tm) appear at top left instead of locations partly outside
  our existing universe
- fixed serious crash happening when reopening a cached window

0.12
----
- fixed attempt to load a random filename as background when there was no
  background prefs set
- rewrote threading system, much more flexible now
- fixed iconcache nuking on some occasions
- added program launching
- now will open a screen on its own
- removed displaying of filenames called ".info", what is it exactly ?
- fixed background positioning, see Matt ? :)
- automatically sets a MagicWB v2 color giving the xen look for MUI apps
- now tries to open its screen as "Workbench" if there's no other Workbench
  screen present

0.13
----
- fixed threads using wrong arguments from time to time, leading to wrong
  backgrounds, icons stopping being loaded, general random mess, etc..
- fixed deadlock when trying to exit under high activity
- no more unecessarily refreshes when resizing a window
- optimized background refresh. Needs latest MUI
- removed resize optimization temporarily as they cause some problems
- now will really replace the Workbench! Put workbench.elf as Extmodule and
  create the 'SYS:System/Ambient' directory. Put the Ambient binary in there
  (with the prefs file if you have one). Reboot in normal Workbench mode (no
  DOpus/Scalos) and Ambient should show up standalone. It's still possible to
  run it the normal way with another Workbench.

0.14
----
- hopefully fixed the crash on exit when many processes were still running
- now full debug is activated when the caps lock key is on so that DOpus users
  can use Ambient as WB replacement when pressing shift
- fixed iconcache refresh problem, thus reenabled refresh optimizations

0.15
----
- fixed refresh not being performed after a drag & drop thus leaving 2 icons
  on the screen
- added path support, project icons work in all cases now
- fixed internal tagitem cloning which could do (harmless) enforcer hits
- fixed methodstack freeing the memory at the wrong time
- fixed major screwup when adding icons to iconview object, it all worked by
  luck
- added internal mungwall system for icon memory allocation, please report if
  you see output from the serial out
- fixed tons of small bugs and memtrashing while attempting to find another
  bug (which is still not found, grr)
- non lockable .info files are reported as such
- fixed path not being passed properly in standalone mode
- uses at least 4096 bytes of stack when running programs
- project icons handling was buggy and would only work depending on the phase
  of the moon and alignement to mekka, fixed
- added 'WBStart' debug menu item
- now runs special projects without file (like SYS:System/Shell.info)

0.16
----
- added some more sanity checks into the tooltype parsing
- added some more checks to the image handling
- fixed some stuff in workbench.library but I forgot what :)
- wrote an icon.library replacement. Uses the same loading routines as
  Ambient. No asynchronous I/O, mempools or other fancy stuffs as the goal is
  to make it compatible. No PutIcon() yet. It "works" for me, your mileage
  will vary, no batteries included, etc.. With the original workbench it makes
  a lot of (read) hits but icons show up. With DOpus it seems to be OK. With
  Ambient.. well, Ambient doesn't care about icon.library anyway so it works.
  Bug reports welcome, especially if you find cases which can be reproduced
  easily

0.17
----
- oops, disabled most of the optimizations in the previous build,
  restored
- fixed small memleak in icon.library
- added support for .backdrop
- added WBStartup support. Location of WBStartup is configurable in the prefs
  and defaults to SYS:WBStartup but it's a good idea to change it if you often
  boot in 68k (for those who like outdated processors :) If an icon has the
  tooltype MOSDONOTSTART, it won't start under MorphOS
- can now be started from... Workbench by double clicking on its icon
- when started from Workbench/CLI it won't process the WBStartup drawer for
  obvious reasons
- fixed some refresh problems when resizing a window whose virtgroup had a
  non null vertical or horizontal starting point. Still some issues
  remaining though

0.18
----
- if a window background takes too long to load and a window is opened before
  it's finished, the background will be added afterwards
- root/window background prefs can be changed immediately now (no need to
  restart Ambient)
- selecting cancel didn't have effect everywhere in the prefs window
- fixed some stupid bugs in the prefs handling.. could have some nasty side
  effects
- icon style and colors can be updated on the fly
- icon cache can be disabled

0.19
----
- fixed launching from workbench.library. Would crash Ambient most of the
  time, sigh
- now states it's loading subsystems when loading the first prefspage

0.20
----
- no longer attempts to execute directories in WBStartup
- shift selection of icons is possible (not drag & drop though)
- added rudimentary context menus, they don't do anything yet
- fixed some icon selection problems after opening a new window then clicking
  back to the parent window
- now handles the argument list differently as SystemTags() without
  SYS_Input, NULL crashes when being done on a bootshell. Is that a
  bug in dosnew ? Is that a correct behaviour ? Dos documentation
  sucks for sure
- won't quit anymore if there are Workbench tasks running
- won't quit anymore if you press 'esc' (or any other key configured to do so)
  on the root window
- icons without position information are added afterwards to prepare for auto
  positioning
- added a setpatch extmodule. Use it to prevent C:SetPatch from patching your
  system and do stupid stuff like blatantly removing workbench.library and
  icon.library resident modules even if their version is higher than the one
  on disk. It's a real shame that something like SetPatch itself, a tool
  designed to fix OS bugs *prevents* OS upgrades
- added a lowmem handler for the icon cache
- dummy entries in the .backdrop files are ignored
- workbench.elf and icon.elf could try to initialize themselves before
  dos.library thus fail on some setups (not mine of course, sigh)
- added basic icon placement which should avoid overlapping for icons without
  x/y coordinates

0.21
----
- some windows could expunge their icon cache without reason
- added more agressive version/revision checking for some libraries
- added info windows for devices, drawers, tools and projects, not fully
  funcional yet, especially the 'Save' function
- loading the root background worked by luck
- device loading is now asynchronous as well
- fixed a major design issue in the threading system. Funny it wasn't noticed
  before. Anyway, some messages would be lost and replied on shutdown when the
  processpool wasn't full causing major side effects like crashes when closing
  a window
- fixed the nasty crash on close which appeared again some versions ago
- added a recursive deletion, pretty untested, only works on directories, in
  the icon's context menu

0.22
----
- fixed some bugs in deletedir which wouldn't delete everything sometimes
- now locks the path to delete before doing so
- root backfilling speeded up by several level of magnitude (or not, it used
  to depend on the CPU charge before)
- raised the maximum number of tooltypes from 256 to 8192, should be enough
  for everyone.. or didn't I say that for 256 already ? :)

0.23
----
- fixed backfilling to work when there's no background configured
- static MUI labels don't waste memory anymore
- now prints "Attempting to launch <progname>..." when attempting to launch a
  program
- fixed the (hopefully) last bugs in deletedir

0.24
----
- added deficon support for devices
- added 'Execute command...' menu function
- added device specification for window output and CLI stack size into the
  prefs
- added snapshot and unsnapshot for icons

0.25
----
- more intelligent positioning system for position-less icons
- oops, Ambient Lock()ed all icons since deficon support without UnLock()ing
  them
- added a confirmation requester for deletion
- deleting something will also delete the .info file
- added file notification support. Due to some heavy braindamage affecting
  some Commodore employees, it doesn't detect when a file was deleted but only
  created
- adding missing defaulttool entry for project icons into the info window
- added mask fixing for original icons so that the background doesn't shine
  through

0.26
----
- fixed race condition bug on exit when there were multiple threads. This
  could leave unfreed signals, leave threads and make random hits
- improved mask fixing for icons
- added layout debugging and image dumping debugging

0.27
----
- memory debugging system had no semaphore protection.. sigh
- added protection for users running in CLUT mode (nowadays no sane person
  runs in CLUT mode :)
- the 2nd image of bordered newicons are loaded (Laire)
- refreshes properly when applying MUI prefs
- now tries to get env:sys/def_device.info then env:sys/def_disk.info for
  devices without icon or deficon rules
- the screen's titlebar tells what is being loaded from WBStartup
- rewrote icon positioning's internal, now simpler
- infowindow displays the icon (only for devices.. have to leave)

0.28
----
- the methodstack won't use the thread's pr_MsgPort anymore, should fix some
  problems although there's more to be done in there
- infowindow displays the icon for everything
- added a scanning of files, directories, soft and hardlinks from the
  infowindow over a directory
- infowindow's creation date is now also shown for devices
- added a 'Version' button into the infowindow which will scan the file for a
  $VER: string and display the result
- the 'Execute command..' requester will now accept URLs if openurl.library is
  installed
- added custom drag & drop system with 3 visual draggings (none of them works
  properly :)
- faster internal prefs handling, should speed up some subsystems
- X/Y icon dragstart is now configurable
- process count indication exit requester won't block Ambient anymore

0.29
----
- fixed X/Y icon dragstart settings not being changed within a session
- fixed most d&d's transparency mode bugs
- trying to exit an Ambient which ran WB programs would produce a buggy
  requester and enforcer hits
- d&d's solid mode now drags without flickering and is optimized for speed
- non-existent entries of a .backdrop file are ignored
- fixed non blocking requester which would happily trash innocent memory
- d&d of directories will move (same partition) or copy them (different
  partition). Please note that you can't d&d over an icon yet
- added d&d marking of icon destination, not configurable yet

0.30
----
- workaround against bigfoot's double click on an icon while clicking once
  outside (tm)
- filecopy doesn't copy hot air anymore
- it's now possible to select the d&d marking style of icon destination from
  the d&d prefs. Available modes are tint, blur, darken and lighten with
  configurable value or color
- d&d of *files* works
- it's possible to delete *files*
- it's now possible to drag files/dirs into a directory's icon

0.31
----
- removed d&d destination highlighting for root desktop
- deleting a directory will also delete empty dirs in it
- iconview's child handling is more efficient
- shift clicking on an iconview won't unselect icons
- infowin doesn't show the global menu anymore
- workbench.library checks if Ambient is correctly installed and warns the
  user if that's not the case
- file notification would not free msgs, wasting memory
- .backdrop files without real objects are displayed anyway
- fixed some bugs in icon.library (enforcer hits, wrong positions, total chaos)
- .backdrop would fail to load the last line if there was no '\n' at the end
  of it
- invalid newicons tooltypes after valid ones could crash the icon loader
- icon.library/GetDiskObject() could leak memory when the icon file wasn't
  found
- icon.library/DeleteDiskObject() would return DOSTRUE/DOSFALSE. Changed to
  return FALSE when DeleteFile() returns DOSTRUE. This makes DOpus happy,
  don't ask me why..
- the icon loader would always trash the end of the DiskObject structure
- added offscreen rendering for transparent and checkered drag & drop, no more
  flickering
- now won't reload icons and add them twice after a d&d operation

0.32
----
- deficon rules wouldn't take the dos device type into account
- added 'def_disk.info' as deficon rule for floppies
- now detects the insertion and removal of floppies/devices
- added progress meter for file/dir copy
- copying and deletion of files is interruptible
- added progress meter for file/dir deletion

0.33
----
- icons only accept files and directories to be dropped on them
- rewrote the whole icon positionning from scratch. Now devices won't
  overlap anymore . Icons without a position are stacked then added
  later where there's empty space. If the window is full, they are
  added on the right of it
- when moving an icon too fast in transparent mode it would be blitted without
  transparency

0.34
----
- disabled internal mungwall
- drag & drop into devices works
- fixed some graphical glitches of drag & drop
- fixed drag&drop selection hotspot
- added drag&drop debugging

0.35
----
- icon clipping was screwed up
- will now display an error requester when trying to delete an illegal
  icon (appicon, device, etc..)
- drag&drop prefs panel would lose its drag&drop display prefs settings
  when switching prefs entry
- added drag&drop ActionKeys (tm) :) Actually they're the same as Windoze,
  so.. if you're dropping on the same device, CTRL switches to copymode and if
  it's another device, shift keys switch to movemode. Only works for
  transparent drag & drop

0.36
----
- moving an image too fast in solid drag & drop move would crash
- fixed the dummy copy of an empty .info when aborting a recursive copy
- fixed random crash during solid/checkered drag & drop dragging
- fixed many bugs in icon.library (Laire)
- added a memhandler for reducing the number of threads
- fixed the icon trashing bug in icon.library, at last :)

0.37
----
- added buffered I/O for icon.library
- upgraded all the modules to extended residents, etc..
- fixed workbench.library crashing when handling most appicons
- added z.alib (it doesn't go to LIBS: ! see docs above)
- added png.alib (it doens't go to LIBS: ! see docs above)
- when there's a foo.info icon, Ambient attempts to load foo.elf, then
 fallsback to foo

0.38
----
- if foo.info runs foo.elf, Ambient tells foo.elf that its name is foo :)
- oops, just broke the project icons loading
- replaced pen allocations by direct true-color rendering
- fixed icon without masking being invisible during drag & drop
- icon.library is only opened if Ambient is started from the WB
- added multiple drag & drop support, use the shift key for multiselection

0.39
----
- fixed simple icon moving that I broke before

0.40
----
- about, prefs, etc.. windows are activated again when they're on screen and
  selected from the menu
- it's not possible to open 2 infowin for the same object
- workbench.library now searches the env var Ambient_path, then
  sys:morphos/ambient, then sys:system/ambient for running Ambient
- progresswins were never disposed until Ambient's termination, causing memory
  waste
- added formatting ('format' context menu). Supports OFS, FFS, MSDOS and SFS
  with most of the filesystem-related options (international, case, recycled,
  show recycled, ..). Quick format, full format, full format + verify. TD64 is
  fully supported

0.41
----
- added some system characteristics in the about window
- now doesn't try to apply the changes when pressing the 'cancel' button in
  the prefs (although there was nothing to apply)
- added a 'Test' button in the prefs
- added 'Grey' and 'Negative' special drag & drop effects
- drag & drop doesn't eat a pen for finding out some colors anymore
- added 'Negative fade' and 'Tint fade' special drag & drop effects

0.42
----
- faster lists handling
- full formatting errors get reported properly
- faster prefs panels switching
- copying files will now also copy the protections, owner and comment, sorry
  Henes :)
- added new doslist handling with non blocking I/O and faster lookup
- copying directories does set the protection, owner, date, etc..
- moving dirs accross partitions wouldn't delete the dirs of the source
- now will offer to run SYS:Prefs/ScreenMode or open a shell if the screen
  can't be opened for some reasons
- when icons are overlapping it's only possible to select one of them
- shift + LMB on the iconview doesn't unselect icons

0.43
----
- wrote a LoadWB command
- the format window could crash on dispose
- implemented renaming of icons

0.44
----
- now sets the PPC stacksize in various parts too
- newicons without a 2nd image are shown properly
- icon.library's GetDiskObject() is now patchable by the newicon patch
- implemented appicons support

0.45
----
- LoadWB doesn't free the copied path on exit anymore (sigh)
- added C: as internal path
- fixed bug in wbstartup handling which could try to execute normal
  tools as project with a random default tool
- fixed some AppIcons crashing when they were added to the desktop
- double clicking on an AppIcon now does something :)

0.46
----
- the stack of Ambient is now set on startup and workbench.library doesn't
  bother about it anymore
- added LoadWB NEWPATH for updating the current path
- MUI Prefs only displays the used custom classes now (but since I don't use
  any.. well, Textinput will appear though)
- now the config file is PROGDIR:config/Ambient.prefs, don't forget to move
  your current one
- thread system uses the new dos/exec extensions for more efficient thread
  management. This removes some quirks as well

[released: 01-08-2002/19:17:13]

0.47
----
- fixed fastlistview not displaying some fonts properly
- some filecopy weren't possible to abort immediately
- defaulttool is only returned for disk/device/project icons
- disk insertion/removal was wrong and could lead to strange side effects
- icon eventhandlers could be added many times
- added recognition and formatting support for BSD disklabels, OSF, Sun,
  Amiga, Atari, Macintosh, Acorn, Sinclair QL, Spectrum, Archimedes, CP/M and
  C64 HDs and floppies
- when moving a directory within a partition, just rename the root dir instead
  of everything
- copying many files doesn't give a progress window with "Moving files..." anymore
- added a safety check for path copy, won't copy locks to files any longer
- now creates a "Workbench" task so that programs relying on it to find
  Ambient's path will work

[released: 13-08-2002/01:32:25]

0.48
----
- readded altivec/fpu infos in the about window
- rearranged the whole debug system. Pressing caps lock on start will
  activate all the debug options but then, they can be turned off from the
  debug menu. There's more debug options available too and it's far easier for
  me to add them now
- rewrote the formatting window. Now can format unmounted devices and offers
  more options. It's not possible to select the filesystem anymore, that is
  done by the partition utility anyway and made no sense here
- added a safety requester for formatting confirmation
- device scanning tries to avoids IsFileSystem() calls as they can be nasty
  with some buggy devices
- it's now possible to delete abnormal icons
- added an internal wbstart.library so that there's no compatibility problems
  anymore with weird versions and clones

[released: 14-08-2002/03:54:05]

0.49
----
- formatwin appears under the mouse pointer when it's opened from a context
  menu or centered if it's from the main menu
- fixed crash which could happen randomly during repeated drag & drop
- added a workaround against the double CON: window opening until con-handler
  gets fixed
- updated the dostype database to recognize between different CDrive CDs
- window event handlers were never removed
- LoadWB didn't work with empty path

[released: 19-08-2002/16:15:11]

0.50
----
- updated the filesystem database for PFS versions
- doslistcache building doesn't send ACTION_IS_FILESYSTEM packets anymore
  as it would start "dormant" handlers. Did I mention I hate buggy handlers
  and devices ?
- added PNGicon support
- fixed wbstart.library's wrong offset because of a missing function

[released: 21-08-2002/02:56:29]

0.51
----
- wbstart.library now really works
- memhandlers would only work once
- tooltypes handling and newicons are a little bit faster now
- now icons are locked to make sure they can't go away while being read
- infowin also opens for icons without an associated file/directory
- added icon saving, phew
- very old icons (1.x stuff) wouldn't be loaded properly
- added icon saving to icon.library as well
- icon shapshot does some sanity checks now

[released: 25-08-2002/05:15:47]

0.52
----
- added a menu to launch NadirPrefs
- added full support for PNGicons (snapshot, tooltypes, launching, info,
  etc..)

[released: 26-08-2002/04:21:54]

0.53
----
- fixed png.alib and z.alib failing to open when currentdir was not set the
  way they expected it to be
- PNGicons know if their location was set
- oops, I broke snapshot for normal icons, fixed
- icon bubble help is also displayed in the infowindow
- tooltypes weren't saved properly, actually they were but I changed a define
  during the final build and didn't notice
- fixed normal image writing which would write trash
- some race conditions could prevent png icons from being saved

[released: 26-08-2002/19:07:05]

0.54
----
- z.alib and png.alib are now mandatory
- fixed some race condition crashes on pngicons loading
- more efficient tooltype writing
- better error reporting for most I/O functions
- WBStartup scanning would abort when there was an empty directory
- infowin works with devices again
- original wbstart.library uses negative logic for its return code.. fixed..
  grr
- WBStartup launching could crash under some rare circumstances, so rare that
  it probably never did
- added some more silly stats to the about window
- rewrote wb launching from scratch, now the wbstart.library emulation handles
  all the cases properly
- Ambient refuses to exit if something is using wbstart.library
- 'TOOLPRI' works for icons now

[released: 29-08-2002/20:55:43]

0.55
----
- png writing won't do a crc check if chunks weren't modified
- added png reading support to icon.library
- added a default image in icon.library for fallback purposes
- added support for 'CLI' tooltypes.. in that case, the program is launched
  just like a CLI program without WB startup message
- added id/userdata to appmessages
- now the context menu is a bit smarter and only shows relevant entries
- added support for WB path scanning for users who run Ambient from an icon
  (hi Henes!)
- progress windows won't show up for quick operations
- programs started by wbstart.library wouldn't read the stack from the icon,
  fixed
- recursive copy won't abort if some target directory already exist
- recursive move doesn't delete the source if the copying failed

[released: 31-08-2002/18:48:12]

0.56
----
- 'CLI' execute programs would have their seglist removed too early
- programs could lock Ambient when replying to the startup message
- appicons would screw up drag & drop, fixed
- disabled Ambient's exiting in WBR mode
- removed icons don't leave empty layout space anymore
- SIGBREAKF_CTRL_C would bypass the normal exit procedure, fixed
- pngicons have a meaningful context menu again
- now can't be iconified in any way
- can't be fooled with a public screen either
- added new default image in icon.library
- layouting tries to avoid overlapping icons better
- adjusted layout to add icons verticaly as well. It would only extend
  horizontaly for some cases
- cleans up locks properly in case of failure to run Ambient
- in case of failure on startup other than screenmode related, Ambient will
  try to spawn a shell
- added device/file listers

[released: 05-09-2002/02:35:24]

0.57
----
- build mess prevented lister colors from working properly
- it was impossible to select the first listview entry
- saving a glowicon would trash it
- 'CLI' mode wouldn't work
- no need for IconX anymore to run scripts from an icon. Simple protect +s
  the script, add a tool icon and voilà
- dirs in WBStartup would temporarily waste memory
- now sets pr_CurrentDir properly. Would cause problems with some programs

[released: 09-09-2002/22:40:06]

0.58
----
- fixed icon tooltype parsing which could misread entries
- added some more semaphore protection to the doslistcache
- added a small timeout and retries to the doslistcache updating in case the
  DosList is locked for too long
- doslistcache now gracefully handles removal of devices
- fixed semaphore deadlock in device handling which would prevent Ambient from
  exiting as soon as a device was inserted
- insertion/removal of device works again
- fixed launching of files without icons

[released: 10-09-2002/16:07:21]

0.59
----
- added proper transparent dragging for PNGicons
- it's not possible to activate lister entries by clicking on the scrollbar
  borders anymore
- optimized alpha channel handling for some cases
- "Execute command.." menu reports I/O errors
- copying stuff doesn't allocate a buffer for every single copy but does it
  once
- added different file copy strategies: fixed: uses a configurable buffer
  size; deviceblock: uses the same buffer size as the greatest logical device
  blocksize of the 2 devices involved; incremental: starts with a buffer size
  of the greatest logical device blocksize of the 2 devices involved and
  doubles it until it reaches the configured buffer size. Also the
  asynchronous I/O readahead prefetch size is configurable
- hopefully fixed the bug that would prevent drag & drop copy to ram:

[released: 13-09-2002/04:36:04]

0.60
----
- icon labels are centered for icons without positioning information too
- dropping destinations give a more realistic dropping zone
- icon.library tries to find out the type of a PNGicon
- Ambient now automatically corrects wrong icon types internally
- PNGicons have a proper icon type

[released: 15-09-2002/00:10:11]

0.61
----
- workbench.library now tells IPrefs to free the fonts it sends us
- Ambient no longers shows 2 tasks in 'status'
- selected icons would show the unselected imagery when being a target of a
  drag&drop operation
- added proper drag&drop destination effect for PNGicons
- adjusted drag&drop move/unknown/copy imagery placement
- Ambient's main process is set to priority 1 to avoid performance problems
  when running CPU intensive programs
- changed the ScreenMode fallback to run SystemPrefs instead of ScreenMode
- icon.library now sets the icon type of unexisting sys:disk.info to WBDEVICE
  to make laire happy
- ah well, since he still wasn't happy he removed my code for that and now he
  is :)
- now shell executed commands are quoted
- optimized window refresh

[released: 28-09-2002/16:48:49]

0.62
----
- icons's selection zone is more accurate now
- device icons can be snapshoted again
- icon.library now has built-in default icons for every icontype
- dostypes containing garbage characters aren't printed anymore
- added "show all icons" mode
- internal command string parser wouldn't parse arguments at all
- 'unsnapshot' works for PNGicons
- added snapshot for windows
- added window unsnapshot
- fixed memleak in icon.library when loading PNGicons

[released: 14-10-2002/02:34:44]

0.63
----
- fixed 'execute command' to not interpret the whole string as a command
- updated 'about' requester
- fixed some weird crashes with icons
- fixed icon.library which was completely broken
- 'Run ScreenMode' in failsafe mode didn't work at all, oops :)
- the icon cache could have issues with dos notification, fixed

[released: 19-10-2002/01:55:48]

