from pynq.overlays.base import BaseOverlay
from pynq.lib.arduino import Arduino
from PIL import Image, ImageDraw
import numpy as np
import time

print("[1/4] Chargement de l'overlay Base sur PYNQ...")
base = BaseOverlay("base.bit")

print("[2/4] Chargement du firmware MicroBlaze...")
tft = Arduino(base.ARDUINO, "/home/xilinx/mri_tft_project/tft_firmware.c")

# Fonction pour envoyer une commande et ses données à la Mailbox MicroBlaze
def send_cmd(cmd, data=[]):
    # Envoi des données dans les registres MAILBOX_DATA (à partir de l'offset 4)
    if data:
        tft.write(0x04, data)
    # Déclenchement de la commande dans MAILBOX_CMD (offset 0)
    tft.write(0x00, [cmd])
    # Attente d'acquittement (le firmware remet MAILBOX_CMD à 0 quand c'est fini)
    while tft.read(0x00) != 0:
        time.sleep(0.001)

print("[3/4] Initialisation de l'ecran TFT 2.4\"...")
send_cmd(0x01) # CMD_INIT
time.sleep(0.3)

# Test 1 : Remplissage en bleu marine (0x08C5)
print("-> Effacement de l'ecran...")
send_cmd(0x02, [0x08C5]) # CMD_CLEAR
time.sleep(0.5)

print("[4/4] Generation de l'interface Dashboard IRM...")
# Image 320x240
img = Image.new("RGB", (320, 240), color=(10, 25, 47))
draw = ImageDraw.Draw(img)

# Cadre IRM
draw.rectangle([10, 30, 190, 210], fill=(30, 45, 70), outline=(0, 230, 118), width=3)
draw.text((30, 110), "[ IMAGE IRM ICI ]", fill=(255, 255, 255))

# Panneau lateral IA
draw.rectangle([200, 30, 310, 210], fill=(23, 42, 69))
draw.text((210, 40), "MODELE IA", fill=(100, 255, 218))
draw.text((210, 70), "STATUS :", fill=(180, 180, 180))
draw.text((210, 90), "AUCUNE", fill=(0, 230, 118))
draw.text((210, 105), "LESION", fill=(0, 230, 118))
draw.text((210, 140), "CONFIANCE :", fill=(180, 180, 180))
draw.text((210, 160), "98.7 %", fill=(255, 255, 255))

# Conversion RGB888 -> RGB565
arr = np.array(img, dtype=np.uint16)
r = (arr[:, :, 0] >> 3) & 0x1F
g = (arr[:, :, 1] >> 2) & 0x3F
b = (arr[:, :, 2] >> 3) & 0x1F
rgb565 = ((r << 11) | (g << 5) | b).flatten().tolist()

# Définition de la fenêtre complète (0,0 à 319,239)
send_cmd(0x03, [0, 0, 319, 239]) # CMD_SET_WIN

# Envoi des pixels par tranches de 256 mots
chunk_size = 256
for i in range(0, len(rgb565), chunk_size):
    chunk = rgb565[i:i + chunk_size]
    send_cmd(0x04, [len(chunk)] + chunk) # CMD_WRITE_BURST

print("==> SUCCES : L'ecran TFT affiche maintenant le dashboard IRM !")
