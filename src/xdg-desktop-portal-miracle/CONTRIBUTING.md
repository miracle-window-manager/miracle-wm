# Contributing

We closely follow the wlroots [contributing] guidelines where possible. Please
see that document for more information.

## Tooling

Useful tools include `dbus-monitor` to watch requests being made,
and `dbus-send` and the similar `busctl call` for manual dbus calls.

You can test the integration with the [portal-test] Flatpak app.

Alternatively you can trigger it with [trigger-screen-shot.py] and
[xdp-screen-cast.py].

[contributing]: https://github.com/swaywm/wlroots/blob/master/CONTRIBUTING.md
[portal-test]: https://github.com/matthiasclasen/portal-test
[trigger-screen-shot.py]: https://gist.github.com/danshick/3446dac24c64ce6172eced4ac255ac3d
[xdp-screen-cast.py]: https://gitlab.gnome.org/snippets/19

## Alternate *.portal Location

xdg-desktop-portal will read the XDG_DESKTOP_PORTAL_DIR environment variable for an
alternate path for *.portal files. This can be useful when testing changes to that
portal file, or for testing xdpw without installing it. This feature is undocumented
and shouldn't be relied on, but may be helpful in some circumstances.

https://github.com/flatpak/xdg-desktop-portal/blob/e7f78640e35debb68fef891fc233c449006d9724/src/portal-impl.c#L124
