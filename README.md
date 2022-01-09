# CppHeaders: C++17 Utility template library

Compiled with

```
- gcc >= 9.3.0
- MSVC >= 19.29.0
```

## INDEX

- [`Reflection`](refl/README.md)

## HOW TO INSTALL

Simply copy whole contents to your own include directory. If you want override namespace identifier,
create `__cppheaders_ns__` file on parent directory of cppheaders include directory, and add definition like below:

```
#define CPPHEADERS_NS_ your_own_namespace
```

> As long as you the namespace identifiers differ, multiple different version of cppheaders can be included on single executable.

Some library features may require explicit compilation or external library dependencies. To make them work, you should
include [`__cpph__.inc`](`__cpph__.inc`) source file somewhere which is compilation unit for single time. Unused
features will automatically be excluded from compilation targets. In that case, using these features may generate linker
error.






