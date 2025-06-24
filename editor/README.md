# Simple Text Editor

This is only for C/C++ geeks, who don't find it necessary to load editors like VSCode just to edit a simple `Marksown` files.

You can compile the codes in `src` folder using a simple command:

```bash
cd src
g++ -s -o editor main.cpp state.cpp \
`pkg-config gtk4 gtksourceview-5 --cflags --libs`
```

There's no fancy stuffs (for now).

