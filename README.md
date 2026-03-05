# Simple operating system template

## Recommended setup

## 1. Clone the repository
```bash
$ git clone https://github.com/TymianekPL/os-template.git
```

### Windows/MSVC
> [!IMPORTANT]
> Remember to run this from your developer command prompt included with Visual Studio.

#### Configuration
Do it after cloning the project, downloads EDK and configures cmake
```batch
scripts\run configure-msvc debug
```
#### Build (& run)
Builds the project, copies files and runs QEMU
```batch
scripts\run build-msvc debug
```

> [!NOTE]
> You can change `debug` to `release` when building the optimised release.
> Remember to reconfigure when switching the flag for the first time!

---

### Advanced setup

### Configuring:

#### Windows x86-64 (MSVC)
```batch
scripts\runx8664 configure-msvc release|debug
```

#### Windows x86-64 (Clang)
```batch
scripts\runx8664 configure-clang release|debug
```

#### Windows x86-32 (MSVC)
```batch
scripts\runx8632 configure-msvc release|debug
```

#### Windows x86-32 (Clang)
```batch
scripts\runx8632 configure-clang release|debug
```

#### Windows ARM64 (MSVC)
```batch
scripts\runarm64 configure-msvc release|debug
```

#### Windows ARM64 (Clang)
```batch
scripts\runarm64 configure-clang release|debug
```

### Building & running:

#### Windows x86-64 (MSVC)
```batch
scripts\runx8664 build-msvc release|debug
```

### Windows x86-64 (Clang)
```batch
scripts\runx8664 build-clang release|debug
```

#### Windows x86-32 (MSVC)
```batch
scripts\runx8632 build-msvc release|debug
```

#### Windows x86-32 (Clang)
```batch
scripts\runx8632 build-clang release|debug
```

#### Windows ARM64 (MSVC)
```batch
scripts\runarm64 build-msvc release|debug
```

#### Windows ARM64 (Clang)
```batch
scripts\runarm64 build-clang release|debug
```
