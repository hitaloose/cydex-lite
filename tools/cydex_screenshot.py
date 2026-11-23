#!/usr/bin/env python3
"""Print da tela do CYD pela serial.

Manda "shot" pro firmware (src/hal/display.cpp, display_capture_to_serial), que
redesenha a tela atual e devolve as faixas do flush do LVGL:
"CYSA" + x, y, w, h (u16 LE) + RGB565 LE + soma (u16), ..., "CYSE". Aqui elas viram um PNG.
O CH340 perde um byte de vez em quando: a faixa com soma errada é descartada e
só aquelas linhas são pedidas de novo ("shot <y0> <y1>").

    pip install pyserial pillow
    python3 tools/cydex_screenshot.py                 # pergunta o nome a cada print
    python3 tools/cydex_screenshot.py vitrine menu    # um print por nome, Enter entre eles

Feche o `pio device monitor` antes (a porta é de um programa só). Abrir a
porta reinicia a placa: abra o script primeiro e só depois navegue até a tela.
A 115200 bps cada print leva uns 13 s.
"""

import argparse
import glob
import os
import struct
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("falta o pyserial: pip install pyserial")
from PIL import Image

W, H = 320, 240
TIMEOUT_S = 30
RETRIES = 3


def default_port():
    for pat in ("/dev/cu.usbserial*", "/dev/cu.wchusbserial*", "/dev/ttyUSB*", "/dev/ttyACM*"):
        found = sorted(glob.glob(pat))
        if found:
            return found[0]
    return None


def open_port(path, baud):
    s = serial.Serial()
    s.port = path
    s.baudrate = baud
    s.timeout = 0.5
    # Antes do open(): menos pulsos no auto-reset. No macOS o open() ainda
    # reinicia a placa, então espera o boot terminar (log parar de chegar).
    s.dtr = False
    s.rts = False
    s.open()
    t0 = last = time.monotonic()
    while time.monotonic() - last < 2 and time.monotonic() - t0 < 15:
        if s.read(4096):
            last = time.monotonic()
    return s


class Reader:
    def __init__(self, port, deadline):
        self.port = port
        self.deadline = deadline
        self.buf = bytearray()

    def need(self, n):
        while len(self.buf) < n:
            if time.monotonic() > self.deadline:
                raise TimeoutError("sem resposta do CYD")
            self.buf += self.port.read(max(n - len(self.buf), 1024))

    def take(self, n):
        self.need(n)
        out = bytes(self.buf[:n])
        del self.buf[:n]
        return out

    def sync(self):
        """Pula log até o próximo magic. Devolve b"CYSA" ou b"CYSE"."""
        while True:
            self.need(4)
            for magic in (b"CYSA", b"CYSE"):
                i = self.buf.find(magic)
                if i >= 0:
                    del self.buf[: i + 4]
                    return magic
            del self.buf[:-3]  # guarda o fim, o magic pode estar cortado


def request(port, cmd, img, done):
    """Manda `cmd` e pinta as faixas que chegarem íntegras. Faixa com byte perdido
    (o CH340/macOS perde bytes de vez em quando) fica de fora: quem chama pede de
    novo só aquelas linhas."""
    # Sobra de um pedido anterior (um "CYSE" atrasado) encerraria este vazio:
    # espera a linha ficar quieta antes de pedir.
    last = time.monotonic()
    while time.monotonic() - last < 0.5:
        if port.read(4096):
            last = time.monotonic()
    port.write(cmd)
    r = Reader(port, time.monotonic() + TIMEOUT_S)
    px = img.load()
    try:
        while r.sync() == b"CYSA":
            x, y, w, h = struct.unpack("<4H", r.take(8))
            if w == 0 or h == 0 or x + w > W or y + h > H:
                continue  # cabeçalho estragado: procura o próximo
            r.need(w * h * 2 + 2)
            data = bytes(r.buf[: w * h * 2])
            (sum_,) = struct.unpack("<H", bytes(r.buf[w * h * 2 : w * h * 2 + 2]))
            vals = struct.unpack(f"<{w * h}H", data)
            if sum(vals) & 0xFFFF != sum_:
                continue  # não consome: o próximo "CYSA" pode estar aí dentro
            del r.buf[: w * h * 2 + 2]
            for j in range(h):
                row = j * w
                for i in range(w):
                    v = vals[row + i]
                    r5, g6, b5 = v >> 11, (v >> 5) & 0x3F, v & 0x1F
                    px[x + i, y + j] = ((r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2))
                if x == 0 and w == W:
                    done[y + j] = True
    except TimeoutError:
        pass  # o "CYSE" se perdeu: fica com o que chegou


def missing_ranges(done):
    out, y = [], 0
    while y < H:
        if done[y]:
            y += 1
            continue
        y0 = y
        while y < H and not done[y]:
            y += 1
        out.append((y0, y - 1))
    return out


def capture(port, log=print):
    img = Image.new("RGB", (W, H))
    done = [False] * H
    request(port, b"shot\n", img, done)
    for attempt in range(1, RETRIES + 1):
        gaps = missing_ranges(done)
        if not gaps:
            return img
        log("  bytes perdidos nas linhas " + ", ".join(f"{a}–{b}" for a, b in gaps) + f"; pedindo de novo ({attempt})")
        for y0, y1 in gaps:
            request(port, f"shot {y0} {y1}\n".encode(), img, done)
    gaps = missing_ranges(done)
    if gaps:
        raise ValueError("faltaram as linhas " + ", ".join(f"{a}–{b}" for a, b in gaps))
    return img


def save(img, out_dir, name, scale):
    if scale != 1:
        img = img.resize((W * scale, H * scale), Image.NEAREST)
    os.makedirs(out_dir, exist_ok=True)
    path = os.path.join(out_dir, name if name.endswith(".png") else name + ".png")
    img.save(path, optimize=True)
    return path


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("names", nargs="*", help="nomes dos prints, em ordem (sem nomes: pergunta)")
    ap.add_argument("--port", default=default_port())
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--scale", type=int, default=2, help="ampliação inteira, vizinho mais próximo (padrão 2)")
    ap.add_argument("--out", default="docs/img")
    a = ap.parse_args()
    if not a.port:
        sys.exit("porta serial não encontrada: use --port")

    port = open_port(a.port, a.baud)
    print(f"porta {a.port}. Ctrl+C sai.")
    queue = list(a.names)
    try:
        while True:
            if a.names:
                if not queue:
                    break
                name = queue.pop(0)
                input(f"deixe a tela de '{name}' no CYD e tecle Enter… ")
            else:
                name = input("nome do print (vazio sai): ").strip()
                if not name:
                    break
            t = time.monotonic()
            try:
                path = save(capture(port), a.out, name, a.scale)
                print(f"  {path} ({time.monotonic() - t:.1f} s)")
            except ValueError as e:
                print(f"  '{name}' não saiu: {e}", file=sys.stderr)
    except (KeyboardInterrupt, EOFError):
        print()
    finally:
        port.close()


if __name__ == "__main__":
    main()
