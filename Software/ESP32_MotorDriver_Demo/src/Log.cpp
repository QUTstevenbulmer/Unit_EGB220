// =============================================================================
//  Log.cpp  -  Sends text to USB serial and keeps the last few kB for the
//              web terminal (a "ring buffer": new text overwrites the oldest).
// =============================================================================
#include "Log.h"
#include "config.h"

LogTee Log;

static char     ring[LOG_BUFFER_BYTES];
static uint32_t total = 0;                 // bytes ever written (position counter)

static inline void keep(uint8_t c) { ring[total % LOG_BUFFER_BYTES] = (char)c; total++; }

size_t LogTee::write(uint8_t c) {
    keep(c);
    return Serial.write(c);
}

size_t LogTee::write(const uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; i++) keep(buf[i]);
    Serial.write(buf, len);
    return len;
}

String LogTee::readSince(long& from) {
    uint32_t oldest = total > LOG_BUFFER_BYTES ? total - LOG_BUFFER_BYTES : 0;
    bool fresh = from < 0 || (uint32_t)from > total;   // new page, or robot restarted
    uint32_t start = fresh ? oldest : max<uint32_t>((uint32_t)from, oldest);
    String s;
    s.reserve(total - start + 40);
    if (!fresh && start > (uint32_t)from) s += "\r\n... (some output was missed) ...\r\n";
    bool skipPartial = fresh && oldest > 0;            // don't start mid-line
    for (uint32_t i = start; i < total; i++) {
        char c = ring[i % LOG_BUFFER_BYTES];
        if (skipPartial) { if (c == '\n') skipPartial = false; continue; }
        s += c;
    }
    from = total;
    return s;
}
