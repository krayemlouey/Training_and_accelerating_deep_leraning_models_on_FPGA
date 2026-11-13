import time
import pynq
from PIL import Image
import numpy as np

# 1. Configuration matérielle du FPGA PYNQ-Z2
pynq.Device.active_device = pynq.Device.devices[0]
import bnn

print("\n" + "=" * 65)
print("🏥 BENCHMARK COMPARATIF : CPU ARM vs ACCÉLÉRATEUR FPGA PYNQ-Z2")
print("=" * 65)

# Création d'une image de test
img = Image.new('RGB', (32, 32), color = (73, 109, 137))

# -------------------------------------------------------------
# TEST 1 : EXÉCUTION LOGICIELLE SUR CPU ARM CORTEX-A9
# -------------------------------------------------------------
print("\n[1/2] 🐢 Test sur CPU ARM Cortex-A9 (Mode Logiciel)...")
sw_classifier = bnn.CnvClassifier(bnn.NETWORK_CNVW1A1, 'cifar10', bnn.RUNTIME_SW)
t0 = time.time()
res_sw = sw_classifier.classify_image(img)
t_cpu = (time.time() - t0) * 1000
print(f"      ⏱️  Temps CPU ARM : {t_cpu:.2f} ms")

# -------------------------------------------------------------
# TEST 2 : EXÉCUTION MATÉRIELLE SUR LE SILICIUM FPGA
# -------------------------------------------------------------
print("\n[2/2] ⚡ Test sur Silicium FPGA Zynq-7020 (Accélération Matérielle)...")
hw_classifier = bnn.CnvClassifier(bnn.NETWORK_CNVW1A1, 'cifar10', bnn.RUNTIME_HW)
t0 = time.time()
res_hw = hw_classifier.classify_image(img)
t_fpga = (time.time() - t0) * 1000
print(f"      ⏱️  Temps FPGA    : {t_fpga:.2f} ms")

# -------------------------------------------------------------
# TABLEAU COMPARATIF RÉCAPITULATIF
# -------------------------------------------------------------
speedup = t_cpu / t_fpga if t_fpga > 0 else 0

print("\n" + "=" * 65)
print("📊 RÉSULTATS COMPARATIFS FINAUX (PYNQ-Z2)")
print("=" * 65)
print(f"  • Architecture CPU ARM       : {t_cpu:.2f} ms")
print(f"  • Architecture Matérielle FPGA : {t_fpga:.2f} ms")
print("-" * 65)
print(f"  🚀 FACTEUR D'ACCÉLÉRATION    : x {speedup:.1f} PLUS RAPIDE SUR FPGA !")
print("=" * 65 + "\n")
