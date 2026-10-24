# SEAL Lab: Encrypted Computing Demo

An educational browser app using Microsoft SEAL 4.7.0. BFV demonstrates exact integer addition and multiplication; CKKS demonstrates approximate vector addition, multiplication, and slot rotation. A small custom Embind interface exposes the operations from WebAssembly, and all computation runs in the visitor's browser.

- **Live demo:** https://seal-encrypted-computing-demo.vercel.app/
- **Source:** https://github.com/ReeshavChowdhury/seal-encrypted-computing-demo

## What it does

- **BFV:** creates a fresh context and key pair, encrypts two integers from 0–999, evaluates exact addition or multiplication, then decrypts locally.
- **CKKS:** creates a fresh context and key pair, encrypts four-value vectors, evaluates approximate slot-wise addition or multiplication, or rotates the encrypted vector. Multiplication uses relinearization and rescaling; rotation generates the Galois key needed for the selected step.

CKKS accepts exactly four finite values between -100 and 100. Positive rotation steps shift left; negative steps shift right. For this four-value view, the pattern repeats through the packed slots so rotation wraps around the displayed vector. CKKS results are approximate. The app does not upload inputs, ciphertexts, or keys. It is an educational example, not a production cryptographic service. Its parameter choices and implementation are intended for demonstration; consult Microsoft's security guidance before building a real system.

## Deploy on Vercel

This is a static site after the WebAssembly artifacts are built. Click the button to connect your Vercel account and import the repository:

[![Deploy with Vercel](https://vercel.com/button)](https://vercel.com/new/clone?repository-url=https%3A%2F%2Fgithub.com%2FReeshavChowdhury%2Fseal-encrypted-computing-demo)

Leave the framework preset as **Other**, leave the build command empty, and use `public` as the output directory (already set in `vercel.json`). The files in `public/` are deployed as-is. Each push to the connected GitHub repository can trigger a new deployment.

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
