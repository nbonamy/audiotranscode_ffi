# audiotranscode_ffi

<hr/>

### **Only built/tested on MacOS for now!**
<hr/>

A dart wrapper for FFmpeg audio conversion.

Supported target formats:
- MP3
- FLAC
- AAC
- ALAC/M4A (Apple Lossless)

## Dependencies

### MacOS

```shell
brew install cmake
brew install ffmpeg
```

## Setup

### MacOS

`HOMEBREW_PREFIX` variable needs to be defined:

```shell
export HOMEBREW_PREFIX="$(brew --prefix)" 
```

## Usage

Add as a `pubspec.yaml` dependency.
