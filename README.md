# Waydroid Runner

This is a helper program for running Waydroid session on Sailfish. It
starts a new session when launched. The runner is the Wayland compositor
that Android draws into, so Android lives as long as the runner does.

## Background mode

This fork (github.com/sailfishos-sony-pdx246/waydroid-runner) keeps
Android running when its window is closed, so that Android's
notifications keep arriving. The launcher shows the window again
without booting Android. Stop Android with the action on the app's
cover in the app switcher; the runner quits once the session has ended
and the window is closed.

- The desktop file has `X-Maemo-Service` keys: the launcher calls
  `activateWindow` on the running instance. If none runs, D-Bus
  activation starts the user unit `waydroid-runner.service`, which runs
  `waydroid-runner -prestart` (window hidden) and gets the same call.
- `waydroid-runner` from a shell shows the running instance, or runs as
  the instance itself if there is none.
- To start Android at login as well:
  `systemctl --user enable waydroid-runner`.

## Android apps

The port's Waydroid package writes a launcher for each Android app
(`~/.local/share/applications/waydroid.<package>.desktop`) that calls
`activateWindow` with the app's package (`X-Maemo-Fixed-Args`, no
`Exec`): the runner starts Android if needed, opens the app with
`waydroid app launch` once Android's UI is up, and shows its window.
Its `app launch` keeps the full UI while it is shown, so the app opens
in the runner's window. A tap on an Android notification calls `Show()`
(Lipstick's remote action `x-nemo-remote-action-default`), while Android
opens the notification's target.

D-Bus interface on the session bus: service and interface
`org.sailfishosopen.WaydroidRunner`, object
`/org/sailfishosopen/WaydroidRunner`.

| Member | |
|---|---|
| `activateWindow(as)` | Launcher entry point: without arguments `Show()`, with an Android app's package first `Launch(package)` |
| `Launch(s)` | Open the Android app with this package (once Android's UI is up) and show the window |
| `Show()` | Show the window; start Android if it does not run |
| `Hide()` | Close the window, as the user would; Android keeps running |
| `Stop()` | Stop the Android session |
| `State` (s) | `stopped`, `starting`, `running`, `stopping` or `error`; signal `StateChanged(s)` |
| `Open` (b) | Whether the window is open, on screen or in the app switcher; signal `OpenChanged(b)` |

For example:
`gdbus call --session --dest org.sailfishosopen.WaydroidRunner --object-path /org/sailfishosopen/WaydroidRunner --method org.sailfishosopen.WaydroidRunner.Hide`

## Acknowledgments

The code is based on QtWayland [example
compositor](https://github.com/qt/qtwayland/tree/5.6/examples/wayland/qml-compositor)
and adaptation of it for Sailfish:
[qxcompositor](https://github.com/elros34/qxcompositor). Code was
first evolved into [Flatpak
Runner](https://github.com/sailfishos-flatpak/flatpak-runner) and
adopted here for managing Waydroid session.

