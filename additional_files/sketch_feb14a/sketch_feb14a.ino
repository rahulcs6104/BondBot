#include <Arduino.h>
#include <driver/i2s.h>
#include <string.h>
#include "test_wav.h"

#define I2S_BCLK 6
#define I2S_LRC  5
#define I2S_DOUT 7

static inline uint16_t u16le(const uint8_t* p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline uint32_t u32le(const uint8_t* p) {
  return (uint32_t)p[0] |
         ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

struct WavInfo {
  uint32_t sampleRate;
  uint16_t bitsPerSample;
  uint16_t channels;
  uint16_t audioFormat; // 1 = PCM
  uint32_t dataOffset;
  uint32_t dataSize;
};

bool parseWav(WavInfo &w) {
  if (test_wav_len < 44) return false;
  const uint8_t* p = test_wav;

  if (!(p[0]=='R'&&p[1]=='I'&&p[2]=='F'&&p[3]=='F')) return false;
  if (!(p[8]=='W'&&p[9]=='A'&&p[10]=='V'&&p[11]=='E')) return false;

  bool gotFmt=false, gotData=false;
  uint32_t pos = 12;

  while (pos + 8 <= test_wav_len) {
    const uint8_t* ch = p + pos;
    uint32_t chunkSize = u32le(ch + 4);
    uint32_t chunkData = pos + 8;
    if (chunkData + chunkSize > test_wav_len) break;

    if (ch[0]=='f'&&ch[1]=='m'&&ch[2]=='t'&&ch[3]==' ') {
      w.audioFormat   = u16le(p + chunkData + 0);
      w.channels      = u16le(p + chunkData + 2);
      w.sampleRate    = u32le(p + chunkData + 4);
      w.bitsPerSample = u16le(p + chunkData + 14);
      gotFmt = true;
    }

    if (ch[0]=='d'&&ch[1]=='a'&&ch[2]=='t'&&ch[3]=='a') {
      w.dataOffset = chunkData;
      w.dataSize   = chunkSize;
      gotData = true;
    }

    pos = chunkData + chunkSize;
    if (chunkSize & 1) pos++;
    if (gotFmt && gotData) return true;
  }
  return false;
}

void i2sStart(uint32_t sr) {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = (int)sr,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num  = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_set_clk(I2S_NUM_0, sr, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
}

void playMonoToStereo(const WavInfo &w) {
  uint32_t end = w.dataOffset + w.dataSize;
  if (end > test_wav_len) end = test_wav_len;

  // Read mono samples and expand to stereo frames (L=R)
  static int16_t mono[256];
  static int16_t stereo[512];

  uint32_t pos = w.dataOffset;
  while (pos + 2 <= end) {
    uint32_t bytesLeft = end - pos;
    uint32_t samples = min((uint32_t)256, bytesLeft / 2);

    memcpy(mono, test_wav + pos, samples * 2);
    pos += samples * 2;

    for (uint32_t i = 0; i < samples; i++) {
      // slightly reduce volume so it stays clean
      int32_t s = mono[i];
      s = (s * 7) / 10;
      int16_t out = (int16_t)s;

      stereo[2*i]     = out;
      stereo[2*i + 1] = out;
    }

    size_t written = 0;
    i2s_write(I2S_NUM_0, stereo, samples * 2 * sizeof(int16_t), &written, portMAX_DELAY);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  WavInfo w;
  if (!parseWav(w)) {
    Serial.println("WAV parse failed.");
    while (1) delay(100);
  }

  Serial.println("=== WAV INFO ===");
  Serial.print("Format: "); Serial.println(w.audioFormat);
  Serial.print("Channels: "); Serial.println(w.channels);
  Serial.print("SampleRate: "); Serial.println(w.sampleRate);
  Serial.print("Bits: "); Serial.println(w.bitsPerSample);

  if (w.audioFormat != 1 || w.bitsPerSample != 16 || w.channels != 1) {
    Serial.println("Please export as PCM 16-bit MONO WAV.");
    while (1) delay(100);
  }

  i2sStart(w.sampleRate);
}

void loop() {
  WavInfo w;
  if (parseWav(w)) {
    playMonoToStereo(w);
  }
  delay(2000);
}
