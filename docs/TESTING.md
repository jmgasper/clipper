# Testing

Run on Haiku:

```sh
make -j8 all check build-haiku/fixture build-haiku/paste_integration
```

`history_tests` checks stable-ID deduplication, newest-first ordering, pin
protection, count eviction, fuzzy search, rich-text preservation, byte-identical
native bitmap roundtrip through persistence, invalid archive rejection, empty
clipboard rejection, and maximum clip size. Temporary archives live under `/tmp`.

`build-haiku/paste_integration` opens a temporary native paste target, captures
rich text and a bitmap, opens Clipper, selects each captured item, and verifies
that the injected paste reaches the original target with every format intact.
The bitmap payload is compared byte for byte. The test also exercises sequential
paste injections, including restoration of modifier state.

The native fixture provides a standard editable text view and overrides its
normal clipboard paste method to archive every payload under
`/tmp/clipper-last-paste`. This checks real input_server-injected shortcuts,
including image formats that an ordinary text view does not display.

```sh
build-haiku/fixture
build-haiku/fixture --set-text 'sample'
build-haiku/fixture --set-image path/to/image.png
build-haiku/fixture --history
build-haiku/fixture --read-paste
build-haiku/fixture --deskbar
build-haiku/fixture --hide
```

For VNC tests, connect to 192.168.1.244:5900 using the workstation credentials.
Keyboard events from its InputEventInjector pass through input_server's filters.
The NanoKVM is an alternate USB keyboard path. Avoid concurrent input from other
clients. Full-frame captures are more reliable than partial captures while
Summit is running a direct-rendered graphics/video benchmark.

Set `CLIPPER_TRACE=1` when launching to log show requests, chosen IDs, destination
team/window, and paste bridge results. This does not log clipboard text.

Manual coverage:

1. Select text in the fixture and use Alt+C. Confirm the history's text and source.
2. Select different text and use Alt+X. Confirm the source text was cut and recorded.
3. Tap Alt+V. Confirm exactly one ordinary paste of the current clip.
4. Hold Alt+V at least 350 ms. Release V and Alt; history opens without pasting.
5. Search an older clip, navigate with arrows, and Enter. Confirm the fixture's
   pasted payload matches that older clip and history closes.
6. Shift+Alt+V opens immediately. Escape closes; no paste occurs.
7. Copy an image. Confirm dimensions, thumbnail, preview, and restored bitmap data
   in `--read-paste`. Also paste into an image-capable application.
8. Pin a clip; recopy it and restart. Confirm its ID stays stable and pin survives.
9. Delete a clip, clear unpinned history, and confirm the pin remains.
10. Pause recording, copy a unique string, resume, and confirm it was not recorded.
11. Disable persistence, quit/restart, and confirm saved history is removed.
12. Open Settings, change the limit and paste preference, save, and restart.
13. Switch focus during paste-back. The app should leave the clip copied and cancel
    injection, rather than paste into a different application.
14. Check the Deskbar icon, its menu, startup link, and absence of duplicate add-ons.

The workstation's previous implementation was removed before installing this
replacement: its executable, boot script, lowercase settings, MIME registration,
Deskbar replicant, running team, and `/boot/home/clipper` source tree. The latter
path now contains the new Clipper source and build.
