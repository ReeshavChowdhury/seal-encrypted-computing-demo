"use strict";

const $ = (id) => document.getElementById(id);
const button = $("compute");
const status = $("status");
let seal;

function setStatus(message, kind = "") {
  status.textContent = message;
  status.className = `status ${kind}`.trim();
}

function numberValue(id) {
  const value = Number($(id).value);
  if (!Number.isInteger(value) || value < 0 || value > 999) {
    throw new Error("Enter whole numbers from 0 to 999.");
  }
  return value;
}

async function boot() {
  try {
    if (typeof createSealModule !== "function") throw new Error("SEAL WebAssembly files did not load.");
    seal = await createSealModule();
    button.disabled = false;
    button.innerHTML = 'Encrypt &amp; compute <span>↗</span>';
    setStatus("SEAL is ready. Your inputs stay in this browser.", "ready");
  } catch (error) {
    setStatus(`Could not load SEAL: ${error.message}`, "error");
  }
}

button.addEventListener("click", () => {
  if (!seal) return;
  button.disabled = true;
  $("result").classList.add("hidden");
  setStatus("Generating keys, encrypting values, and evaluating ciphertexts…");
  // Keep the interface responsive while WebAssembly creates fresh parameters and keys.
  setTimeout(() => {
    try {
      const a = numberValue("a");
      const b = numberValue("b");
      const op = $("op").value;
      const out = seal.compute(a, b, op === "multiply");
      if (out.error) throw new Error(out.error);
      $("resultValue").textContent = out.value.toLocaleString();
      $("operationLabel").textContent = `${a} ${op === "multiply" ? "×" : "+"} ${b}`;
      $("cipherBytes").textContent = `${out.ciphertextBytes.toLocaleString()} ciphertext bytes evaluated`;
      $("result").classList.remove("hidden");
      setStatus("Encrypted evaluation complete. The result was decrypted locally.", "ready");
    } catch (error) {
      setStatus(error.message || "The encrypted computation failed.", "error");
    } finally {
      button.disabled = false;
    }
  }, 30);
});

boot();
