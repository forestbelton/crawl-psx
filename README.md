# crawl-psx
This is a port of Linley's Dungeon Crawl for the Playstation One.

## Build
Make sure you have CMake and [PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK) installed and the `PSN00BSDK_LIBS` environment variable set. Then execute the following commands:

```
$ cmake --preset psx .
$ cmake --build ./build-psx
```

If the build succeeds, the CD image will be present at `./build-psx/crawl.{bin,cue}`.
