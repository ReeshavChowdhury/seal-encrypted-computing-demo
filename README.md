# SEAL Lab: Encrypted Computing Demo

A small educational web application that demonstrates exact integer addition and multiplication with Microsoft SEAL's BFV homomorphic encryption scheme. Microsoft SEAL 4.7.0 is compiled to WebAssembly with a small custom Embind interface; the computation runs in the visitor's browser.

## What it does

1. Creates a fresh BFV context and key pair for each calculation.
2. Encrypts two integers (0–999) with the public key.
3. Adds or multiplies the ciphertexts with the SEAL evaluator.
4. Decrypts the result locally with the secret key.

The app does not upload inputs, ciphertexts, or keys. It is an educational example, not a production cryptographic service. Its parameter choices and implementation are intended for demonstration; consult Microsoft's security guidance before building a real system.

## Deploy on Vercel

This is a static site after the WebAssembly artifacts are built. Import this repository in Vercel, leave the framework preset as **Other**, leave the build command empty, and set the output directory to `public` (already set in `vercel.json`). The files in `public/` are deployed as-is.

Build `public/seal/seal.js` and `public/seal/seal.wasm` before deployment by following the WebAssembly instructions below. They are checked in so Vercel does not need a C++ or Emscripten build environment.

## Build Microsoft SEAL WebAssembly

Use Microsoft SEAL 4.7.0 and Emscripten 6.0.13 (or a compatible Emscripten SDK), plus CMake and Make/Ninja:

```bash
git clone --depth 1 --branch v4.7.0 https://github.com/microsoft/SEAL.git vendor/SEAL
export EMSDK=/path/to/emsdk
export SEAL_SRC="$PWD/vendor/SEAL"
./scripts/build-wasm.sh
```

The build outputs `public/seal/seal.js` and `public/seal/seal.wasm`. The official SEAL repository explains its WebAssembly build and warns that the library does not provide built-in JavaScript bindings; this project supplies a narrow custom binding for this demo.

## References

- [Microsoft SEAL](https://github.com/microsoft/SEAL)
- [Microsoft SEAL WebAssembly build instructions](https://github.com/microsoft/SEAL#building-for-webassembly)
- [Microsoft SEAL security guidance](https://github.com/microsoft/SEAL/blob/main/SECURITY.md)
