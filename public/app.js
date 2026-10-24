"use strict";

const $ = (id) => document.getElementById(id);
const button = $("compute");
const status = $("status");
const ckksButton = $("compute-ckks");
const ckksStatus = $("ckks-status");
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

function vectorValue(id) {
  const values = $(id).value.split(",").map((part) => Number(part.trim()));
  if (values.length !== 4 || values.some((value) => !Number.isFinite(value) || Math.abs(value) > 100)) {
    throw new Error("Enter exactly four finite numbers between -100 and 100.");
  }
  return values;
}

function formatApprox(value) {
  const rounded = value.toFixed(5).replace(/\.?0+$/, "");
  return rounded === "-0" ? "0" : rounded;
}

function setScheme(scheme) {
  const bfv = scheme === "bfv";
  $("tab-bfv").classList.toggle("active", bfv);
  $("tab-bfv").setAttribute("aria-selected", String(bfv));
  $("tab-ckks").classList.toggle("active", !bfv);
  $("tab-ckks").setAttribute("aria-selected", String(!bfv));
  $("bfv-panel").classList.toggle("hidden", !bfv);
  $("ckks-panel").classList.toggle("hidden", bfv);
}

function updateCkksOperation() {
  const rotate = $("ckks-op").value === "rotate";
  $("ckks-b-wrap").classList.toggle("hidden", rotate);
  $("rotation-wrap").classList.toggle("hidden", !rotate);
}

$("tab-bfv").addEventListener("click", () => setScheme("bfv"));
$("tab-ckks").addEventListener("click", () => setScheme("ckks"));
$("ckks-op").addEventListener("change", updateCkksOperation);

async function boot() {
  try {
    if (typeof createSealModule !== "function") throw new Error("SEAL WebAssembly files did not load.");
    seal = await createSealModule();
    button.disabled = false;
    ckksButton.disabled = false;
    button.innerHTML = 'Encrypt &amp; compute <span>↗</span>';
    ckksButton.innerHTML = 'Encrypt &amp; compute <span>↗</span>';
    setStatus("SEAL is ready. Your inputs stay in this browser.", "ready");
    ckksStatus.textContent = "SEAL is ready. CKKS will return approximate values.";
    ckksStatus.className = "status ready";
  } catch (error) {
    setStatus(`Could not load SEAL: ${error.message}`, "error");
    ckksStatus.textContent = `Could not load SEAL: ${error.message}`;
    ckksStatus.className = "status error";
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

ckksButton.addEventListener("click", () => {
  if (!seal) return;
  ckksButton.disabled = true;
  $("ckks-result").classList.add("hidden");
  ckksStatus.textContent = "Generating CKKS keys, encrypting vectors, and evaluating ciphertexts…";
  ckksStatus.className = "status";
  setTimeout(() => {
    try {
      const operation = $("ckks-op").value;
      const a = vectorValue("ckks-a");
      const b = operation === "rotate" ? [] : vectorValue("ckks-b");
      const steps = Number($("rotation-steps").value);
      if (!Number.isInteger(steps) || steps < -3 || steps > 3) {
        throw new Error("Rotation steps must be a whole number from -3 to 3.");
      }
      const out = seal.computeCkks(a, b, { add: 0, multiply: 1, rotate: 2 }[operation], steps);
      if (out.error) throw new Error(out.error);
      $("ckks-result-value").textContent = `[${out.values.map(formatApprox).join(", ")}]`;
      const labels = { add: "A + B", multiply: "A × B", rotate: `Rotate A ${steps > 0 ? `${steps} left` : `${Math.abs(steps)} right`}` };
      $("ckks-operation-label").textContent = labels[operation];
      $("ckks-cipher-bytes").textContent = `${out.ciphertextBytes.toLocaleString()} ciphertext bytes evaluated`;
      $("ckks-result").classList.remove("hidden");
      ckksStatus.textContent = "Encrypted evaluation complete. CKKS output is approximate and was decrypted locally.";
      ckksStatus.className = "status ready";
    } catch (error) {
      ckksStatus.textContent = error.message || "The CKKS computation failed.";
      ckksStatus.className = "status error";
    } finally {
      ckksButton.disabled = false;
    }
  }, 30);
});

boot();
