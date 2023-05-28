# CX - C Extended

I've made CX, because C does not allow many features out of the box or is scarse in others. CX parses files in my custom `CX` language, implementing non-native features as needed and translates the code to pure C.

## Quick start

As for now the only possible output of the code is the AST, it can be seen usign the `--dump-ast` flag like so

```console
$ gcc -o cx cx.c
$ ./cx test.cx --dump-ast | jq
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