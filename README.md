# picard

Yet another pica200 shader binary disassembler.

## Usage

```
Usage: picard [options] files
Options:
  -D, --dvle <index>    Specifies the dvle to print info from
  -V, --verbose         Enables printing of verbose messages
  -v, --version         Displays version information
  -h, --help            Displays this message
```

## Building

```
meson build
meson compile -C build
```

## Credits

+ Fincs: for creating picasso, his reworks to libctru's shader handling, hard work on reverse engineering the pica200 shader binary format, and for writing the picasso manual, all of which were incredibly helpful.
+ Smealum: for kickstarting the reverse engineering effort of the pica200 including writing aemstro, which was still referenced a few times and libctru, which was referenced heavily.
+ Neobrain: for his effort with reverse engineering the pica200, his contributions to 3dbrew, and nihstro-disassemble, which was used to compare against.
+ Others: 3dbrew was referenced quite heavily so big thanks to the contributors for taking the time out of their day to document the pica200 among other stuff.
