# Workstation validation — 2026-10-02

Built and tested on 192.168.1.244, x86_64 Haiku hrev60097+178 with the air/OS
workstation build. Both the Linux cross-build and native Haiku build completed
without compiler warnings. The native package was created and its contents
verified, including the executable, both uniquely named input_server add-ons,
application menu link, MIME resources, and documentation.

Verified:

- History unit tests: deduplication, stable IDs, pins, eviction, fuzzy search,
  text/HTML preservation, bitmap byte roundtrip, atomic save/load, corrupt data,
  empty clipboard, and oversize clips.
- VNC copy and cut from a normal native text view populate history automatically.
- VNC tap Alt+V with the installed filter yields exactly one ordinary paste.
- Hold Alt+V opens history; search and Enter paste an older clip back into the
  original native window. Shift+Alt+V is also recognized.
- Native paste integration sends both text/HTML and images through the registered
  paste device into the original target. Restored bitmap bytes match the source.
  Sequential injections leave modifier state available for the next paste.
- Pin retention across a graceful quit/restart, pause/resume, saved preferences,
  clear unpinned while keeping pins, deletion, and persistence disabled across
  restart.
- Deskbar replicant registration and rendering, history and image preview layout,
  startup link, and one loaded shortcut filter plus one loaded paste device.

The earlier implementation was stopped and removed. Test history was cleared
and default preferences restored after verification. Clipper is left running in
its normal background mode with login startup enabled.

Image display/paste remains destination-dependent: the integration view proves
that image data reaches the original destination through the same keyboard paste
path; applications must support the supplied clipboard format to display it.
