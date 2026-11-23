#!/usr/bin/env python3
"""Simula o Claude Desktop (Hardware Buddy) pra testar o CYDex Lite sem o app.

Procura "Claude-CYDex-*", manda time/owner, pede status e roda um roteiro de
heartbeats: ocioso -> rodando -> pedido de permissão (espera você aprovar ou
negar no CYD) -> ocioso. Imprime tudo o que o CYD responde.

    python3 -m venv .venv && .venv/bin/pip install bleak
    .venv/bin/python tools/buddy_fake_desktop.py            # roteiro
    .venv/bin/python tools/buddy_fake_desktop.py --unpair   # esquece o pareamento

No 1º acesso o macOS abre o pedido de pareamento: digite o código que aparece
na tela do CYD (L03). O Terminal pede permissão de Bluetooth na 1ª vez.
"""
import argparse
import asyncio
import json
import time

from bleak import BleakClient, BleakScanner

NUS = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  # escrita (desktop -> CYD)
TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # notify (CYD -> desktop)


class Link:
    def __init__(self, client):
        self.client = client
        self.buf = b""
        self.lines = asyncio.Queue()

    def on_notify(self, _char, data: bytearray):
        self.buf += bytes(data)
        while b"\n" in self.buf:
            line, self.buf = self.buf.split(b"\n", 1)
            print(f"  <- {line.decode(errors='replace')}")
            try:
                self.lines.put_nowait(json.loads(line))
            except json.JSONDecodeError:
                print("     (JSON inválido)")

    async def send(self, obj):
        data = (json.dumps(obj, ensure_ascii=False) + "\n").encode()
        print(f"  -> {data.decode().rstrip()[:160]}")
        n = max(20, self.client.mtu_size - 3)
        for off in range(0, len(data), n):
            await self.client.write_gatt_char(RX, data[off : off + n], response=True)

    async def expect(self, key, value, timeout=10):
        end = time.monotonic() + timeout
        while True:
            left = end - time.monotonic()
            if left <= 0:
                raise TimeoutError(f"sem {key}={value}")
            msg = await asyncio.wait_for(self.lines.get(), left)
            if msg.get(key) == value:
                return msg


def snapshot(running=0, waiting=0, tokens=184502, today=31200, prompt=None):
    s = {
        "total": 2,
        "running": running,
        "waiting": waiting,
        "msg": "approve: Bash" if prompt else ("working" if running else "idle"),
        "entries": ["10:42 git push", "10:41 yarn test"],
        "tokens": tokens,
        "tokens_today": today,
    }
    if prompt:
        s["prompt"] = prompt
    return s


async def heartbeat_for(link, seconds, snap):
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        await link.send(snap)
        await asyncio.sleep(min(10, max(0.1, end - time.monotonic())))


async def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--unpair", action="store_true", help="manda {\"cmd\":\"unpair\"} e sai")
    ap.add_argument("--wait", type=int, default=60, help="segundos esperando a decisão no CYD")
    a = ap.parse_args()

    print("procurando Claude-CYDex-* ...")
    dev = await BleakScanner.find_device_by_filter(
        lambda d, ad: (d.name or ad.local_name or "").startswith("Claude-CYDex-"), timeout=15
    )
    if not dev:
        raise SystemExit("não achei: o CYD está ligado e longe de outro computador conectado?")
    print(f"achei {dev.name} ({dev.address})")

    async with BleakClient(dev) as client:
        link = Link(client)
        # O CCCD do TX é cifrado: aqui o macOS pede o passkey da tela do CYD.
        await client.start_notify(TX, link.on_notify)
        print(f"conectado, mtu={client.mtu_size}")

        if a.unpair:
            await link.send({"cmd": "unpair"})
            await link.expect("ack", "unpair")
            print("pareamento esquecido no CYD (esqueça também nos Ajustes de Bluetooth do macOS)")
            return

        tz = -time.altzone if time.localtime().tm_isdst > 0 else -time.timezone
        await link.send({"time": [int(time.time()), tz]})
        await link.send({"cmd": "owner", "name": "Teste"})
        await link.expect("ack", "owner")
        await link.send({"cmd": "status"})
        st = await link.expect("ack", "status")
        print(f"status: {json.dumps(st.get('data'), ensure_ascii=False)}")
        await link.send({"cmd": "char_begin", "name": "x", "total": 10})  # não deve ter ack
        await link.send({"evt": "turn", "role": "assistant", "content": [{"type": "text", "text": "oi"}]})

        print("\n1) ocioso (5 s)")
        await heartbeat_for(link, 5, snapshot())
        print("2) rodando (8 s)")
        await heartbeat_for(link, 8, snapshot(running=1, tokens=190000, today=36700))

        print(f"3) pedido de permissão: aprove ou negue no CYD (até {a.wait} s)")
        prompt = {"id": "req_fake_001", "tool": "Bash", "hint": "rm -rf /tmp/foo && git push origin main"}
        snap = snapshot(running=0, waiting=2, prompt=prompt)
        end = time.monotonic() + a.wait
        decision = None
        while decision is None and time.monotonic() < end:
            await link.send(snap)
            try:
                decision = await link.expect("cmd", "permission", timeout=min(10, end - time.monotonic()))
            except TimeoutError:
                pass
        if decision:
            ok = decision.get("id") == prompt["id"]
            print(f"   decisão: {decision.get('decision')} (id {'confere' if ok else 'ERRADO'})")
        else:
            print("   sem decisão")

        print("4) heartbeat atrasado com o mesmo pedido (não pode reabrir), depois ocioso")
        await link.send(snap)
        await asyncio.sleep(1)
        await heartbeat_for(link, 5, snapshot(tokens=240000, today=86700))  # passa de nível

        await link.send({"cmd": "status"})
        st = await link.expect("ack", "status")
        print(f"status: {json.dumps(st.get('data'), ensure_ascii=False)}")
        print("fim. Sem heartbeat, o CYD mostra Desconectado em ~30 s.")


if __name__ == "__main__":
    asyncio.run(main())
