#pragma once
#include <M5Cardputer.h>
#include <NimBLEDevice.h>
#include <atomic>
#include "romaji.h"

namespace mx11 {
constexpr int width = 384;
constexpr int lineHeight = 24;

inline uint8_t crc(const uint8_t* data, size_t size, uint8_t value = 0) {
    while (size--) {
        value ^= *data++;
        for (int bit = 0; bit < 8; ++bit)
            value = (value << 1) ^ ((value & 0x80) ? 0x07 : 0);
    }
    return value;
}

inline bool send(NimBLERemoteCharacteristic& tx, uint8_t command,
                 const uint8_t* payload, size_t size, size_t mtuPayload, bool imageProtocol = false) {
    uint8_t frame[56] = {uint8_t(imageProtocol ? 0x22 : 0x51),
                         uint8_t(imageProtocol ? 0x21 : 0x78), command, 0, uint8_t(size), 0};
    if (size) memcpy(frame + 6, payload, size);
    frame[6 + size] = crc(payload, size);
    frame[7 + size] = 0xff;
    if (command != 0xa2) {
        Serial.print("MX11 TX");
        for (size_t i = 0; i < size + 8; ++i) Serial.printf(" %02X", frame[i]);
        Serial.println();
    }
    for (size_t offset = 0; offset < size + 8; offset += mtuPayload) {
        size_t bytes = std::min(mtuPayload, size + 8 - offset);
        if (!tx.writeValue(frame + offset, bytes, tx.canWrite() &&
                          (command != 0xa2 || !tx.canWriteNoResponse()))) {
            Serial.printf("MX11 GATT write failed for %02X\n", command);
            return false;
        }
        delay(12);
    }
    return true;
}

inline const char* print(NimBLEClient& client, const String& note, uint32_t id, bool classic) {
    auto* service = client.getService(NimBLEUUID(uint16_t(0xae30)));
    if (!service) service = client.getService(NimBLEUUID(uint16_t(0xaf30)));
    if (!service) return "Printer service unavailable";
    auto* image = service->getCharacteristic(NimBLEUUID(uint16_t(0xae03)));
    if (classic) image = nullptr;
    auto* tx = service->getCharacteristic(NimBLEUUID(uint16_t(0xae01)));
    auto* rx = service->getCharacteristic(NimBLEUUID(uint16_t(0xae02)));
    if (!tx || (!tx->canWrite() && !tx->canWriteNoResponse()) || !rx || !rx->canNotify())
        return "Printer channels unavailable";

    static std::atomic<unsigned> expected{0};
    static std::atomic<int> reply{-1};
    static std::atomic<bool> imageProtocol{false};
    static std::atomic<unsigned> notifications{0};
    notifications.store(0);
    imageProtocol.store(image != nullptr);
    expected.store(0);
    Serial.printf("MX11 connected, MTU=%u, image=%d, write=%d, writeNoResponse=%d\n",
                  client.getMTU(), image != nullptr, tx->canWrite(), tx->canWriteNoResponse());
    if (!rx->subscribe(true, [](NimBLERemoteCharacteristic*, uint8_t* data, size_t n, bool) {
        ++notifications;
        Serial.printf("MX11 RX (%u)", unsigned(n));
        for (size_t i = 0; i < std::min(n, size_t(64)); ++i) Serial.printf(" %02X", data[i]);
        Serial.println();
        const bool separateImage = imageProtocol.load();
        if (n < 7 || data[0] != (separateImage ? 0x22 : 0x51) ||
            data[1] != (separateImage ? 0x21 : 0x78) || data[2] != expected.load()) return;
        size_t length = data[4] | (size_t(data[5]) << 8);
        if (!length || length > n - 6) return;
        if (data[2] == 0xa1) {
            if (length >= 9) {
                bool ready = data[6] == 0 && data[10] <= 38 && data[12] == 0 &&
                             (length < 10 || data[15] == 0);
                reply.store(ready ? 0 : 1);
            }
        } else if (data[2] == 0xaa) {
            if (length >= 2) reply.store(data[7]);
        } else {
            reply.store(data[6]);
        }
    })) return "Printer status unavailable";

    size_t payload = client.getMTU() - 3;
    const auto request = [&](uint8_t cmd, const uint8_t* data, size_t size,
                             uint8_t response, uint32_t timeout) {
        reply.store(-1);
        expected.store(response);
        if (!send(*tx, cmd, data, size, payload, imageProtocol.load())) {
            expected.store(0);
            return -1;
        }
        uint32_t started = millis();
        while (reply.load() < 0 && client.isConnected() && millis() - started < timeout) delay(20);
        expected.store(0);
        return reply.load();
    };

    const uint8_t zero = 0;
    if (image) {
        if (!send(*tx, 0xb1, &zero, 1, payload, true)) return "MX11 initialization failed";
        delay(50);
    }
    int status = image ? request(0xa1, &zero, 1, 0xa1, 3000) :
                         request(0xa3, &zero, 1, 0xa3, 3000);
    if (status < 0 && image && client.isConnected()) {
        imageProtocol.store(false);
        status = request(0xa3, &zero, 1, 0xa3, 3000);
        if (status >= 0) image = nullptr;
    }
    const bool funPrint = image && status < 0 && notifications.load() == 0 && client.isConnected();
    if (funPrint) imageProtocol.store(true);
    if (status < 0 && !funPrint && !classic) return notifications.load() ? "Unknown MX11 status. See USB log." :
                                                 "No MX11 notification. See USB log.";
    if (status > 0) return "MX11 busy or check paper";
    if (image && (payload < (funPrint ? 24 : width / 8) || (!image->canWrite() && !image->canWriteNoResponse())))
        return "MX11 image channel unavailable";

    M5Canvas line(&M5Cardputer.Display);
    line.setColorDepth(1);
    if (!line.createSprite(width, lineHeight)) return "No memory for print image";
    line.setFont(&fonts::efontJA_10);
    line.setTextSize(2);
    line.setTextColor(TFT_WHITE);
    std::vector<String> lines;
    lines.push_back("SAISHUBAKO #" + String(id));
    String current;
    for (unsigned i = 0; i < note.length();) {
        unsigned start = i++;
        while (i < note.length() && utf8Continuation(note[i])) ++i;
        String character = note.substring(start, i);
        if (line.textWidth(current + character) > width - 16) {
            lines.push_back(current);
            current = "";
        }
        current += character;
    }
    if (!current.isEmpty()) lines.push_back(current);
    if (funPrint) {
        lines.emplace_back();
        lines.emplace_back();
    }
    const unsigned rows = lines.size() * lineHeight;
    if (classic) {
        const uint8_t quality = 0x32, drawingMode = 1, speed = 25;
        const uint8_t lattice[] = {0xaa, 0x55, 0x17, 0x38, 0x44, 0x5f, 0x5f, 0x5f, 0x44, 0x38, 0x2c};
        const uint8_t energy[] = {0xe0, 0x2e};
        if (!send(*tx, 0xa4, &quality, 1, payload) ||
            !send(*tx, 0xa6, lattice, sizeof(lattice), payload) ||
            !send(*tx, 0xaf, energy, sizeof(energy), payload) ||
            !send(*tx, 0xbe, &drawingMode, 1, payload) ||
            !send(*tx, 0xbd, &speed, 1, payload)) return "MX11 setup failed";
    }
    if (image) {
        uint8_t start[] = {uint8_t(rows), uint8_t(rows >> 8), width / 8, uint8_t(funPrint ? 0 : 1)};
        if (funPrint) {
            if (!send(*tx, 0xa9, start, sizeof(start), payload, true)) return "MX11 start failed";
            delay(50);
        } else if (request(0xa9, start, sizeof(start), 0xa9, 3000) != 0) {
            return "MX11 declined print";
        }
    }

    uint8_t imageCrc = 0;
    for (const auto& text : lines) {
        line.fillScreen(TFT_BLACK);
        line.drawString(text, 8, 1);
        for (int y = 0; y < lineHeight; ++y) {
            uint8_t row[width / 8] = {};
            for (int x = 0; x < width; ++x)
                if (line.readPixel(x, y)) row[x / 8] |= 1 << (x % 8);
            if (!client.isConnected()) return "Print interrupted";
            if (image) {
                const bool acknowledged = !image->canWriteNoResponse();
                const bool sent = funPrint ?
                    image->writeValue(row, 24, acknowledged) &&
                    image->writeValue(row + 24, 24, acknowledged) :
                    image->writeValue(row, sizeof(row), acknowledged);
                if (!sent)
                    return "Image transfer failed";
                imageCrc = crc(row, sizeof(row), imageCrc);
                delay(14);
            } else if (!send(*tx, 0xa2, row, sizeof(row), payload)) {
                return "Print interrupted";
            }
        }
    }

    uint8_t feed[] = {48, 0};
    if (funPrint) {
        if (!send(*tx, 0xad, &zero, 1, payload, true)) return "MX11 flush failed";
        delay(std::max(2500U, rows * 14U));
        return "Sent to MX11. Check paper.";
    }
    if (image) {
        int reported = request(0xad, &zero, 1, 0xaa, 8000 + rows * 70);
        if (reported < 0) return "Print completion not confirmed";
        if (reported != imageCrc) return "Print image check failed";
        if (request(0xa3, feed, sizeof(feed), 0xa3, 4000) < 0) return "Printed; feed not confirmed";
        return "Printed on MX11";
    }
    if (classic) {
        const uint8_t lattice[] = {0xaa, 0x55, 0x17, 0, 0, 0, 0, 0, 0, 0, 0x17};
        if (!send(*tx, 0xa6, lattice, sizeof(lattice), payload)) return "MX11 finish failed";
    }
    if (!send(*tx, 0xa1, feed, sizeof(feed), payload)) return "Paper feed failed";
    delay(300);
    return "Sent to MX11";
}

inline const char* printNote(const String& note, uint32_t id, bool classic) {
    static bool initialized = false;
    if (!initialized) {
        NimBLEDevice::init("");
        NimBLEDevice::setMTU(128);
        initialized = true;
    }
    auto* scan = NimBLEDevice::getScan();
    scan->setActiveScan(true);
    auto results = scan->start(4, false);
    const char* result = "MX11 not found. Check power.";
    for (int i = 0; i < results.getCount(); ++i) {
        auto printer = results.getDevice(i);
        if (printer.getName() != "MX11") continue;
        auto* client = NimBLEDevice::createClient();
        if (!client) { result = "Cannot create BLE connection"; break; }
        client->setConnectTimeout(5);
        result = client->connect(&printer) ? print(*client, note, id, classic) : "MX11 connection failed";
        if (client->isConnected()) client->disconnect();
        NimBLEDevice::deleteClient(client);
        break;
    }
    scan->clearResults();
    return result;
}
}
