#include "ble/ble_link.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <esp_mac.h>
#include <esp_random.h>

#include "buddy/LineAssembler.h"
#include "util/log.h"

static const char *TAG = "ble";

#define NUS_SERVICE_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define NUS_RX_UUID "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define NUS_TX_UUID "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

// Anel de RX: o callback (task do NimBLE) escreve, o tick (loop) lê.
static const size_t kRxCap = 2048;
static uint8_t s_rx[kRxCap];
static volatile size_t s_head = 0, s_tail = 0;
static volatile uint32_t s_dropped = 0;

static char s_name[20];
static NimBLEServer *s_server = nullptr;
static NimBLECharacteristic *s_tx = nullptr;
static volatile bool s_connected = false;
static volatile bool s_secure = false;
static volatile uint32_t s_passkey = 0;
static volatile uint16_t s_mtu = 23;
static volatile uint16_t s_conn = 0xFFFF;
static volatile uint32_t s_events = 0;  // incrementado pelo NimBLE; o tick vira versão
static uint32_t s_version = 1, s_seen_events = 0;
static LineAssembler s_lines;

static void rx_push(const uint8_t *p, size_t n) {
  for (size_t i = 0; i < n; i++) {
    size_t next = (s_head + 1) % kRxCap;
    if (next == s_tail) {
      s_dropped += (uint32_t)(n - i);
      return;
    }
    s_rx[s_head] = p[i];
    s_head = next;
  }
}

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c) override {
    NimBLEAttValue v = c->getValue();
    rx_push(v.data(), v.length());
  }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *, ble_gap_conn_desc *desc) override {
    s_connected = true;
    s_conn = desc->conn_handle;
    s_secure = desc->sec_state.encrypted && desc->sec_state.authenticated;
    s_events++;
  }
  void onDisconnect(NimBLEServer *) override {
    s_connected = false;
    s_secure = false;
    s_passkey = 0;
    s_mtu = 23;
    s_conn = 0xFFFF;
    s_events++;
    NimBLEDevice::startAdvertising();
  }
  void onMTUChange(uint16_t mtu, ble_gap_conn_desc *) override { s_mtu = mtu; }
  // DisplayOnly: o stack pede o passkey pra mostrar; o desktop digita.
  uint32_t onPassKeyRequest() override {
    uint32_t pk = 100000 + esp_random() % 900000;
    s_passkey = pk;
    s_events++;
    return pk;
  }
  void onAuthenticationComplete(ble_gap_conn_desc *desc) override {
    s_passkey = 0;
    s_secure = desc->sec_state.encrypted && desc->sec_state.authenticated;
    s_events++;
    // Pareamento falhou ou foi sem MITM: derruba (os dados exigem cifra).
    if (!s_secure) NimBLEDevice::getServer()->disconnect(desc->conn_handle);
  }
};

void ble_link_init() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT);
  snprintf(s_name, sizeof(s_name), "Claude-CYDex-%02X%02X", mac[4], mac[5]);

  LOG_I(TAG, "antes do init: heap=%u maior_bloco=%u", ESP.getFreeHeap(),
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  NimBLEDevice::init(s_name);
  NimBLEDevice::setMTU(517);
  NimBLEDevice::setSecurityAuth(true, true, true);  // bonding, MITM, Secure Connections
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);

  s_server = NimBLEDevice::createServer();
  s_server->setCallbacks(new ServerCallbacks());
  NimBLEService *svc = s_server->createService(NUS_SERVICE_UUID);
  s_tx = svc->createCharacteristic(
      NUS_TX_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN);
  NimBLECharacteristic *rx = svc->createCharacteristic(
      NUS_RX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::WRITE_ENC |
                       NIMBLE_PROPERTY::WRITE_AUTHEN);
  rx->setCallbacks(new RxCallbacks());
  svc->start();

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE_UUID);
  adv->setScanResponse(true);  // o nome vai na resposta (UUID de 128 bits + nome não cabem em 31 B)
  adv->start();
  LOG_I(TAG, "anunciando \"%s\": heap=%u maior_bloco=%u", s_name, ESP.getFreeHeap(),
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

void ble_link_tick(BleLineHandler on_line) {
  uint32_t ev = s_events;
  if (ev != s_seen_events) {
    s_seen_events = ev;
    s_version++;
    LOG_I(TAG, "%s%s mtu=%u heap=%u maior_bloco=%u", s_connected ? "conectado" : "desconectado",
          s_secure ? " (cifrado)" : s_passkey ? " (pareando)" : "", (unsigned)s_mtu,
          ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    if (!s_connected) s_lines.reset();
  }
  while (s_tail != s_head) {
    uint8_t b = s_rx[s_tail];
    s_tail = (s_tail + 1) % kRxCap;
    if (s_lines.push(b) && on_line) on_line(s_lines.line(), s_lines.length());
  }
}

const char *ble_link_name() { return s_name; }
bool ble_link_connected() { return s_connected; }
bool ble_link_secure() { return s_secure; }
uint32_t ble_link_passkey() { return s_passkey; }
uint32_t ble_link_version() { return s_version; }

void ble_link_send_line(const char *line) {
  if (!s_tx || !s_connected) return;
  size_t len = strlen(line);
  size_t chunk = s_mtu > 3 ? s_mtu - 3 : 20;
  // A linha e o '\n' saem em pedaços de MTU−3; o desktop junta até o '\n'.
  static uint8_t buf[520];
  if (chunk > sizeof(buf)) chunk = sizeof(buf);
  size_t total = len + 1;
  for (size_t off = 0; off < total; off += chunk) {
    size_t k = total - off < chunk ? total - off : chunk;
    for (size_t i = 0; i < k; i++) buf[i] = off + i < len ? (uint8_t)line[off + i] : '\n';
    s_tx->setValue(buf, k);
    s_tx->notify();
  }
}

void ble_link_unpair() {
  LOG_I(TAG, "esquecendo pareamentos");
  uint16_t conn = s_conn;
  NimBLEDevice::deleteAllBonds();
  if (s_server && conn != 0xFFFF) s_server->disconnect(conn);
}
