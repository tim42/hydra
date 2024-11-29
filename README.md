![hydra](data/images/hydra-logo.png)

---

**this is not the renderer you are looking for**


---

hydra: C++23ish vulkan renderer(ish).

---

**Disclaimer**: the only supported platform is linux (not unix, linux), eventhough other unix platforms
can be accomodated for at a loss of feature. (And windows can be supported when the liburing equivalent is available)

 - Hydra uses lib_uring for asynchronous IO. Other POSIX platforms can fallback to a fully synchronous IO
   (they should support scatter/gatherIO, otherwise it's going to be synchronous AND slower than necessary).
   (synchronous fallback is not done as of yet).

I do not own a window/bsd/\*nix machine with a window/bsd/\*nix dev env, and I don't plan to.
As such I am unable to add proper windows/bsd/\*nix support.

## Engine Features

 - Fully asynchronous and easy to use IO. (and _only_ asynchronous IO)
 - A modern, flexible (and fully asynchronous) resource system
 - Fully task based. Most things will run in parallel.
 - A very nice and modern renderer
 - A nice way to handle configuration (replaces console variables & conf in most engines), albeit with a binary format.
   (It uses RLE + metadata. But the format is self descripting and generates nice UI / can be logged / ...)
 - Automatic UI generation from C++ types. (Again, it uses RLE facilities for that).

This project is under development, and can't be used in any kind of project.
Yet.

Also keep in mind that this project is mainly for me a way to learn vulkan, so
the early code may be... somewhat strange and quite odd.

---

## How to build:
```
git submodule init
git submodule update

mkdir build && cd build && cmake ..
make
```

---


