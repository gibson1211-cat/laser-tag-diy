#include "tag_net.h"
#include "secrets.h"

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <time.h>

static WiFiClient net;
static PubSubClient mqtt(net);

static char hostname_buf[32];
static char device_id_buf[24];
static char kind_buf[12];
static char match_id_buf[40] = "null";

static unsigned long last_wifi_try = 0;
static unsigned long last_mqtt_try = 0;
static bool play_enabled = true;   // bench default; HA/match can turn off
static bool match_mode_off = false;
static bool play_entity_off = false;
static bool ntp_ok = false;

static const char *TOPIC_EVENT = "tag/event";
static const char *TOPIC_MATCH = "tag/match";
static const char *TOPIC_CMD = "ha/cmd";
static const char *TOPIC_STATE = "ha/state";

static void iso_now(char *buf, size_t n) {
  time_t t = time(nullptr);
  if (t < 1700000000) {
    snprintf(buf, n, "%lu", (unsigned long)millis());
    return;
  }
  struct tm tm;
  localtime_r(&t, &tm);
  strftime(buf, n, "%Y-%m-%dT%H:%M:%S", &tm);
}

static bool night_hours() {
  if (!ntp_ok) {
    time_t t = time(nullptr);
    if (t < 1700000000) {
      return false;
    }
    ntp_ok = true;
  }
  time_t t = time(nullptr);
  struct tm tm;
  localtime_r(&t, &tm);
  int h = tm.tm_hour;
  // Kid-safe: no loud audio 20:00-08:00 America/New_York.
  return (h >= 20 || h < 8);
}

static void join_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname_buf);
  // Visible house SSID this week (ORBI75). Do not use hidden scan_ssid (Arena_Net is later).
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

static void ensure_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  unsigned long now = millis();
  if (now - last_wifi_try < 8000) {
    return;
  }
  last_wifi_try = now;
  join_wifi();
}

static bool json_str_field(const char *json, const char *key, char *out, size_t outlen) {
  char pat[48];
  snprintf(pat, sizeof(pat), "\"%s\"", key);
  const char *p = strstr(json, pat);
  if (!p) {
    return false;
  }
  p = strchr(p + strlen(pat), ':');
  if (!p) {
    return false;
  }
  p++;
  while (*p == ' ') {
    p++;
  }
  if (strncmp(p, "null", 4) == 0) {
    snprintf(out, outlen, "null");
    return true;
  }
  if (*p != '"') {
    return false;
  }
  p++;
  size_t i = 0;
  while (*p && *p != '"' && i + 1 < outlen) {
    out[i++] = *p++;
  }
  out[i] = 0;
  return true;
}

static void apply_match_json(const char *json) {
  char mode[16] = "";
  char state[16] = "";
  char mid[40] = "";
  json_str_field(json, "mode", mode, sizeof(mode));
  json_str_field(json, "state", state, sizeof(state));
  if (json_str_field(json, "match_id", mid, sizeof(mid))) {
    strncpy(match_id_buf, mid, sizeof(match_id_buf) - 1);
    match_id_buf[sizeof(match_id_buf) - 1] = 0;
  }
  match_mode_off = (strcmp(mode, "off") == 0) || (strcmp(state, "stopped") == 0);
}

static void apply_ha_state_json(const char *json) {
  char entity[64] = "";
  char state[16] = "";
  json_str_field(json, "entity_id", entity, sizeof(entity));
  json_str_field(json, "state", state, sizeof(state));
  if (strcmp(entity, "input_boolean.laser_tag_play") == 0) {
    play_entity_off = (strcmp(state, "on") != 0);
  }
}

static void apply_ha_cmd_json(const char *json) {
  char entity[64] = "";
  char service[48] = "";
  json_str_field(json, "entity_id", entity, sizeof(entity));
  json_str_field(json, "service", service, sizeof(service));
  if (strcmp(entity, "input_boolean.laser_tag_play") == 0) {
    if (strstr(service, "turn_on")) {
      play_entity_off = false;
    } else if (strstr(service, "turn_off")) {
      play_entity_off = true;
    }
  }
}

static void on_mqtt(char *topic, byte *payload, unsigned int len) {
  char buf[512];
  if (len >= sizeof(buf)) {
    len = sizeof(buf) - 1;
  }
  memcpy(buf, payload, len);
  buf[len] = 0;

  if (strcmp(topic, TOPIC_MATCH) == 0) {
    apply_match_json(buf);
  } else if (strcmp(topic, TOPIC_STATE) == 0) {
    apply_ha_state_json(buf);
  } else if (strcmp(topic, TOPIC_CMD) == 0) {
    apply_ha_cmd_json(buf);
  }
}

static void ensure_mqtt() {
  if (mqtt.connected()) {
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  unsigned long now = millis();
  if (now - last_mqtt_try < 4000) {
    return;
  }
  last_mqtt_try = now;
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(30);
  mqtt.setBufferSize(512);
  mqtt.setCallback(on_mqtt);
  const char *user = (MQTT_USER[0] == '\0') ? nullptr : MQTT_USER;
  const char *pass = (MQTT_PASSWORD[0] == '\0') ? nullptr : MQTT_PASSWORD;
  if (mqtt.connect(hostname_buf, user, pass)) {
    mqtt.subscribe(TOPIC_MATCH, 1);
    mqtt.subscribe(TOPIC_CMD, 1);
    mqtt.subscribe(TOPIC_STATE, 1);
    // Do not subscribe to vision/event.
  }
}

static void publish_event(const char *json) {
  if (!mqtt.connected()) {
    return;
  }
  mqtt.publish(TOPIC_EVENT, json, false);
}

static bool ota_started = false;

static void ensure_ota() {
  if (WiFi.status() != WL_CONNECTED) {
    ota_started = false;
    return;
  }
  if (ota_started) {
    ArduinoOTA.handle();
    return;
  }
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname(hostname_buf);
  if (OTA_PASSWORD[0] != '\0') {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }
  ArduinoOTA.begin();
  ota_started = true;
  Serial.printf("OTA 3232 host=%s ip=%s mac=%s\n",
                hostname_buf,
                WiFi.localIP().toString().c_str(),
                WiFi.macAddress().c_str());
}

void tagNetBegin(const char *hostname, const char *device_id, const char *kind) {
  strncpy(hostname_buf, hostname, sizeof(hostname_buf) - 1);
  strncpy(device_id_buf, device_id, sizeof(device_id_buf) - 1);
  strncpy(kind_buf, kind, sizeof(kind_buf) - 1);
  hostname_buf[sizeof(hostname_buf) - 1] = 0;
  device_id_buf[sizeof(device_id_buf) - 1] = 0;
  kind_buf[sizeof(kind_buf) - 1] = 0;

  mqtt.setSocketTimeout(2);
  join_wifi();

  // America/New_York — NTP is async; IR loop does not wait on it.
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void tagNetLoop() {
  ensure_wifi();
  if (WiFi.status() == WL_CONNECTED) {
    ensure_ota();
    ensure_mqtt();
    if (mqtt.connected()) {
      mqtt.loop();
    }
  } else {
    ota_started = false;
  }
  play_enabled = !match_mode_off && !play_entity_off;
}

void tagNetOnFire() {
  char ts[40];
  iso_now(ts, sizeof(ts));
  char json[320];
  if (strcmp(match_id_buf, "null") == 0) {
    snprintf(json, sizeof(json),
             "{\"match_id\":null,\"type\":\"fire\",\"gun_id\":\"%s\",\"ts\":\"%s\"}",
             device_id_buf, ts);
  } else {
    snprintf(json, sizeof(json),
             "{\"match_id\":\"%s\",\"type\":\"fire\",\"gun_id\":\"%s\",\"ts\":\"%s\"}",
             match_id_buf, device_id_buf, ts);
  }
  publish_event(json);
}

void tagNetOnHit(const char *shooter_id, const char *target_id, int lives) {
  char ts[40];
  iso_now(ts, sizeof(ts));
  const char *sid = (shooter_id && shooter_id[0]) ? shooter_id : "unknown";
  const char *tid = (target_id && target_id[0]) ? target_id : device_id_buf;
  char json[384];
  const char *mid_fmt_null = "{\"match_id\":null,\"type\":\"hit\",\"gun_id\":\"%s\",\"target_id\":\"%s\",\"lives\":%d,\"ts\":\"%s\"}";
  const char *mid_fmt = "{\"match_id\":\"%s\",\"type\":\"hit\",\"gun_id\":\"%s\",\"target_id\":\"%s\",\"lives\":%d,\"ts\":\"%s\"}";
  if (lives < 0) {
    if (strcmp(match_id_buf, "null") == 0) {
      snprintf(json, sizeof(json),
               "{\"match_id\":null,\"type\":\"hit\",\"gun_id\":\"%s\",\"target_id\":\"%s\",\"ts\":\"%s\"}",
               sid, tid, ts);
    } else {
      snprintf(json, sizeof(json),
               "{\"match_id\":\"%s\",\"type\":\"hit\",\"gun_id\":\"%s\",\"target_id\":\"%s\",\"ts\":\"%s\"}",
               match_id_buf, sid, tid, ts);
    }
  } else if (strcmp(match_id_buf, "null") == 0) {
    snprintf(json, sizeof(json), mid_fmt_null, sid, tid, lives, ts);
  } else {
    snprintf(json, sizeof(json), mid_fmt, match_id_buf, sid, tid, lives, ts);
  }
  publish_event(json);
}

bool tagNetPlayEnabled() { return play_enabled; }

bool tagNetAudioAllowed() { return play_enabled && !night_hours(); }

bool tagNetMqttConnected() {
  return WiFi.status() == WL_CONNECTED && mqtt.connected();
}

bool tagNetWifiConnected() { return WiFi.status() == WL_CONNECTED; }

const char *tagNetDeviceId() { return device_id_buf; }
const char *tagNetHostname() { return hostname_buf; }
const char *tagNetMatchId() { return match_id_buf; }
