This file contains a variety of development notes. I'll keep my FIXMEs
and TODOs in here.

# Engine

We should consider disabling some _unsafe_ Lua built in functions such as
`dofile()`, `load()` and `loadfile()`, etc.

It appears that _Sandbox_ is the term you're looking for.
See http://lua-users.org/wiki/SandBoxes. Also read the discussion
of the topic at http://stackoverflow.com/q/966162/115589 and
http://stackoverflow.com/q/1224708/115589

It seems that the accepted solution is to not call `luaL_openlibs()`
and rather copying and modifying linit.c according to your needs.

The state files should not have any global variables. I need the states to
be reentrant, so that you can push and pop states from a stack. This is
useful in, say, an adventure game scenario where the town map is a state
and the player enters a shop which is another state that gets pushed on
the states stack. If the player then leaves the shop, the shop state is
popped, so that the player is back in town where he left.

This pushing and popping of states has some implications for the resource
cache: The graphics used in the shop should be released if the player
leaves, but not the town's graphics. This implies that either

1. the resource cache should also work on a stack that gets pushed
and popped,

2. the resource cache should be a part of `struct game_state`.

I'm thinking that (2) might be the more elegant solution at the moment,
but I should think on it a bit more.

This scheme of pushing/popping states also has some implications on the
save game system. And on the sprite system I intend to add later.

My Bitmap module should have a `#pragma pack()` at the
structure declaration. The PAK file module should have
it too. See the bottom of this section on the wikipedia:
http://en.wikipedia.org/wiki/Data_structure_alignment in the section
"Typical alignment of C structs on x86" (Seems it is not a big GCC issue,
rather a MSVC thing)

Speaking of the PAK file module, the ZIP file format turns out to be
not that different, so I should look into it more closely.

Also, the new SDL_RWops may save you a lot of time getting resources
from PAK files later.

The SDL_RWops API is here:
http://wiki.libsdl.org/SDL_RWops?highlight=%28\bCategoryStruct\b%29|%28CategoryIO%29

The bmp.c can do with an `bm_arc(bmp, start_angle, end_angle, radius)`
function.

bmp.c is also still missing a `bm_fillellipse()` function.

~~Descision whether to stick with scancodes or switch to keycodes? Or
make it configurable?~~

Scratch this whole idea: I should have a way to map physical input
(like key pressed, mouse moved or screen swiped) to an abstracted input
(like move left, jump, open door). This will help if I ever get around
to porting it to devices without keyboards and mouses.

Docs says `SDL_UpdateTexture()` be slow. Do something different.

I don't like the fact that you have to `strdup()` the string you
return to Musl in the case of an external Musl function returning a
string. The interpreter should be modified so that Musl does a `strdup()`
after calling the external function in `fparams()` in musl.c (look for
`v->v.fun()`).

The current way of doing it has its pros, so I should think about it
first before committing to a course of action. The best place to think
about it is where I actually use the API (which at the moment is Rengine)

I still have to put audio into the engine. At the moment I have SDL_mixer 2.0.0 built,
but without any support for additional file formats.

# Editor:

FIXME: Save on exit prompt.

Changing the Working Directory affects the tilesets, so you should rather
set the working directory when you start a new project. I should simply
disable the menu option after you've started editing the level. This
implies that there should be a `SetDirty()` function that changes the
titlebar, causes the user to be prompted for a save on exit, and disables
the set working directory menu option.

I should actually have the tilesets be part of the map structure. This
will help with managing resources when I implement the push/pop of game
states later. _Are the tileset bitmaps obtained through the resources
module?_

A menu option to reload the current tileset bitmap. The use case is that
you may have the file open in a paint program while you're editing your
level, and want to see how your changes affect the level.

# Other Tools

## pakker

I should bundle my **pakker** program (that creates the PAK files)'s
code with Rengine's.

**Pakker** should also be scriptable. I forsee that manually adding files
to PAKs will become unwieldy as the projects grow, so having a script to
create PAK files could help. At the moment I think Musl with the pak API.