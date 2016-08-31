![YägGLer logo](data/images/hydra-logo.png)


# `hydra`: C++ vulkan renderer.

---

## this is not the renderer you are looking for

---

This project is under development, and can't be used in any kind of project.
Yet.

Also keep in mind that this project is mainly for me a way to learn vulkan, so
the early code may be... somewhat strange and quite odd.

## Features

- ultra-verbose / comprehensive error reporting / debug logging
  (can log each call to vulkan that return a value)
- some kind of a vulkan wrapper, with some missing features and bad
  implementation at some places. (wait, is _that_ a feature ?).
- A super nice vulkan instance and device creator that is way easier to use
  than the ultra-verbose vulkan way
- a nice (yet untested) scheduler designed to be extremely adaptable to most
  usages (with a thread pool ? ok. Oh and I have a thread with 5ms of free time ?
  Ok. That task should only run on those two thread, depending on whose has less
  work, and with a delay of 12ms. Ok. Oh and If a thread have some free time, I
  would like to perform some background task without blocking or slowing down
  anything, just for using a bit more the CPU when there's nothing else. Ok.
  Is that also possible to tell me if I should add another thread ? Ok). And
  much more.
- An utility to batch transfers (currently, it only handle buffers, but may
  soon handle images
- Header only. (ok, that may be a bad idea and make crash my IDE).

Hydra isn't tied to a window system, and is designed to have almost no
dependencies (excepted a C++14 compiler, vulkan and glm), and thus can be used
as a offline renderer. It provides nevertheless a GLFW extension that allows
and easy window creation and management.

And more importantly, it allow you to perform most vulkan tutorials in less than
~150 lines while providing the same functionality.

## Future

Broken code will hopefully be fixed ; a higher level "layer" will be present ;
PBR / PPR extensions ; texture and model loader extensions ;
YägGLer application-like concept ; and much more.

## Non-Future

_What I don't want in hydra_


Copy. For most objects, copy (affectation or copy construction) is disabled.
The only thing you can do use the move sematic on objects (mostly via
constructors, sometimes via affectation).<br/>
(copy can be a heavy or meaningless operation when done with the = operator,
as, with vulkan, most of the time you require a queue, a command buffer,
sometime synchronisation and memory barriers for transitions. To keep things
simples, you can't copy except by explicitly calling a method for that).


Having a non-extensible / non-easily modifiable API: you want to do
something directly with vulkan ? ok. You then want to use the result of that
operation with hydra ? ok. You better know how to use vulkan, but that will
work.

This also means that hydra can't be a heavy interface-based API and will expose
/ be shaped around some vulkan particularities.


---

## How to build:
```
git submodule init
git submodule update

mkdir build && cd build && cmake ..
make
```

---


Made w/ <3 by Timothée Feuillet.
