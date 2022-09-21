# Fuzzing on transaction parser and formatter

## Compilation

In `fuzz` folder

```
cmake -DCMAKE_C_COMPILER=clang -Bbuild -H.
```

then

```
make -C build
```

## Run

```
./build/fuzz_tx
```