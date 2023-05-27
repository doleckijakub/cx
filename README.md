# CX - C Extended

Since C does not allow many features or is scarse in some I've made this simple program. It parses files in my custom `CX` language, implementing non-native features as needed and translates the code to pure C.

## Quick start

```console
$ gcc -o cx cx.c
$ ./cx test.cx > test.c
$ gcc -o test test.c
$ ./test
```

## Implemented features


- [ ] default function parameters
- [ ] try/catch statements
- [ ] compile-time calculations
- [ ] templates
- [ ] garbade collenction
- [ ] OOP
- [ ] variadics
- [ ] all in one build system