# crawl-psx
This is a port of Linley's Dungeon Crawl for the Playstation One.

## Build
Make sure you have CMake and [PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK) installed and the `PSN00BSDK_LIBS` environment variable set. Then execute the following commands:

```
$ cmake --preset default .
$ cmake --build ./build
```

If the build succeeds, the CD image will be present at `./build/crawl.{bin,cue}`.