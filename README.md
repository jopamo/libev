# libev

High-performance event loop/model with multiple backends and low overhead.

## Building with Meson

```sh
mkdir -p build
meson setup build
meson compile -C build
meson install -C build    # optional, requires privileges
```

Sources live in `src/` and public headers in `include/`.

## About

Libev is modeled (loosely) after libevent and the Event Perl module, aiming to be faster, more scalable, more correct, and smaller.

Highlights include:

- Extensive, readable documentation (not doxygen).
- Fork support with automatic re-arming of kernel mechanisms.
- Optimized select, poll, epoll, Linux AIO, kqueue, and Solaris event ports backends.
- Filesystem path watching (optional Linux inotify).
- Wallclock-based (absolute) timers and relative timers resilient to time jumps.
- Fast inter-thread loop communication (optional Linux eventfd backend).
- Small, dependency-light, easy to embed; optional autoconf.
- Low memory use and extensible embedding of other loops/users.
- Optional C++ interface with zero overhead, and third-party bindings (C++, D, Ruby, Python, etc.).
- Optional Perl interface capable of running Glib/Gtk2 on libev.

Examples of projects embedding libev: EV Perl module, node.js, auditd, rxvt-unicode, gvpe, Deliantra MMORPG server, Rubinius VM, Ebb web server, Rev event toolkit.

## Original Contributors

Libev was written and designed by Marc Lehmann and Emanuele Giaquinta.

Notable contributors (patches/design input):

- W.C.A. Wijngaards
- Christopher Layne
- Chris Brody
