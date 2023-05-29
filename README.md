# CX - C Extended

I've made CX, because C does not allow many features out of the box or is scarse in others. CX parses files in my custom `CX` language, implementing non-native features as needed and translates the code to pure C.

## Quick start

Below is an example of how a `test.cx` file can be translated to `test.c` and then compiled with gcc.

```console
$ make
$ ./cx test.cx -o test.c
$ gcc test.c -o test
```

## Implemented features


- [ ] self hosting
- [ ] default function parameters
- [ ] try/catch statements
- [ ] compile-time calculations
- [ ] templates
- [ ] garbade collenction
- [ ] OOP
- [ ] variadics
- [ ] all in one build system