# Design

The `BApplication` owns history and settings on its own looper thread. It watches
`be_clipboard` for `B_CLIPBOARD_CHANGED`, snapshots the complete `BMessage` under
the clipboard lock, and records changes without intercepting copy or cut events.
Selecting history commits the complete payload and marks its clipboard commit
count so Clipper does not recapture its own restoration.

History contains IDs, copy timestamps, source labels, pin flags, and complete
clipboard messages. Recopying the same content moves its stable ID to newest;
pins remain pinned. Oldest unpinned clips are evicted by count or byte limit.
A bounded versioned archive is saved using a private temporary file, file sync,
and atomic rename. A corrupt archive is reported; capturing new content replaces it on the next save.
Disabling persistence removes the saved history while retaining the live session.

The window takes its own snapshot while locked and rebuilds previews when visible. Its list groups pins first;
search uses a case-insensitive subsequence match. Native bitmap archives and
Translation Kit image decoding supply thumbnails and the preview. All clipboard
formats are kept for restoration, independently of the preview renderer.

`Clipper_shortcuts` is an input_server filter. Copy and cut events pass unchanged.
For Alt+V, the filter saves key-down, starts a 350 ms one-shot timer on a separate
looper, and swallows repeated key-downs. A quick key-up replays the original paste
key-down and key-up through input_server. Holding the key opens history and
consumes the matching key-up; Shift+Alt+V opens immediately. IPC and launching
happen on the dispatcher thread. Stale timers carry generation IDs and cannot
open a picker after release. The filter honors mapped `raw_char` for layouts.

`Clipper_paste` is a registered keyboard input device controlled via
`BInputDevice::Control`. It injects one marked Alt+V event pair, which the shortcut
filter passes unchanged. The app remembers the front normal window before showing
history, restores that window, checks the destination team is still active, and
waits briefly for physical modifiers to release before requesting injection.
Clipboard restoration works even if the device isn't available.

The device and filter intentionally use different filenames: Haiku's shared
add-on monitor resolves filename precedence across the input_server directories.
The Deskbar replicant is archived from the executable and exports its
`Instantiate` function out of line so Haiku can discover it.
