#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <SocketIOclient.h>

const char* ssid = "Dz";
const char* password = "htmlcssjs";
const char* serverUrl = "http://127.0.0.1:4000";
const int micPin = 34;
const int durasiRekam = 5;
const int intervalUnggah = 30000;
const unsigned long intervalHapus = 600000;
const char* idPerangkat = "000000001";
const int pinBuzzer = 14;

WiFiClient klien;
SocketIOclient socket;
static const uint8_t PIN_MP3_TX = 26;
static const uint8_t PIN_MP3_RX = 27;
SoftwareSerial serialSoftware(PIN_MP3_RX, PIN_MP3_TX);
DFRobotDFPlayerMini pemutar;

void setup() {
  Serial.begin(115200);
  serialSoftware.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Terhubung ke WiFi");

  if (!pemutar.begin(serialSoftware)) {
    Serial.println("DFPlayer Mini tidak terdeteksi");
    while(true);
  }
  pemutar.volume(20);
  pemutar.play(1);

  pinMode(micPin, INPUT);
  pinMode(pinBuzzer, OUTPUT);

  socket.begin("127.0.0.1", 4000);
  socket.on("trigger_buzzer", triggerBuzzer);
  
  kirimIDPerangkat();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    static unsigned long waktuUnggahTerakhir = 0;
    static unsigned long waktuHapusTerakhir = 0;

    if (millis() - waktuUnggahTerakhir >= intervalUnggah) {
      waktuUnggahTerakhir = millis();
      rekamDanUnggahAudio();
    }

    if (millis() - waktuHapusTerakhir >= intervalHapus) {
      waktuHapusTerakhir = millis();
      hapusRekamanLama();
    }
  } else {
    Serial.println("WiFi tidak terhubung");
  }
  
  socket.loop();
}

void rekamDanUnggahAudio() {
  File audioFile = SD.open("/rekam/rekam.wav", FILE_WRITE);
  if (audioFile) {
    Serial.println("Merekam audio...");
    long waktuMulai = millis();

    byte header[44];
    memset(header, 0, sizeof(header));
    
    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';
    
    // Placeholder for ChunkSize
    
    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';
    
    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';
    
    header[16] = 16;
    
    header[20] = 1;
    
    header[22] = 1;
    
    header[24] = 16000 & 0xFF;
    header[25] = (16000 >> 8) & 0xFF;
    header[26] = (16000 >> 16) & 0xFF;
    header[27] = (16000 >> 24) & 0xFF;
    
    header[28] = 32000 & 0xFF;
    header[29] = (32000 >> 8) & 0xFF;
    header[30] = (32000 >> 16) & 0xFF;
    header[31] = (32000 >> 24) & 0xFF;
    
    header[32] = 2;
    
    header[34] = 16;
    
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';
    
    audioFile.write(header, sizeof(header));

    int sampleCount = 0;

    while (millis() - waktuMulai < durasiRekam * 1000) {
      int nilaiMic = analogRead(micPin);
      audioFile.write((byte)(nilaiMic & 0xFF));
      audioFile.write((byte)((nilaiMic >> 8) & 0xFF));
      sampleCount++;
    }

    long fileSize = 36 + sampleCount * 2;
    long dataSize = sampleCount * 2;

    audioFile.seek(4);
    audioFile.write((byte)(fileSize & 0xFF));
    audioFile.write((byte)((fileSize >> 8) & 0xFF));
    audioFile.write((byte)((fileSize >> 16) & 0xFF));
    audioFile.write((byte)((fileSize >> 24) & 0xFF));

    audioFile.seek(40);
    audioFile.write((byte)(dataSize & 0xFF));
    audioFile.write((byte)((dataSize >> 8) & 0xFF));
    audioFile.write((byte)((dataSize >> 16) & 0xFF));
    audioFile.write((byte)((dataSize >> 24) & 0xFF));

    audioFile.close();
    Serial.println("Audio direkam");
    unggahAudio("/rekam/rekam.wav");
  } else {
    Serial.println("Gagal membuka file untuk merekam");
  }
}

void unggahAudio(const char* namaFile) {
  if (SD.exists(namaFile)) {
    HTTPClient http;
    http.begin(klien, String(serverUrl) + "/audio");
    http.addHeader("Content-Type", "audio/wav");
    http.addHeader("Device-ID", idPerangkat);
    http.setTimeout(10000);

    File audioFile = SD.open(namaFile);
    if (!audioFile) {
      Serial.println("Gagal membuka file untuk diunggah");
      return;
    }

    int ukuranFile = audioFile.size();
    uint8_t* buffer = new uint8_t[ukuranFile];
    audioFile.read(buffer, ukuranFile);
    audioFile.close();

    int kodeResponHttp = http.POST(buffer, ukuranFile);
    if (kodeResponHttp > 0) {
      String respon = http.getString();
      Serial.println("Respon server: " + respon);
      if (respon.indexOf("mengumpat") > 0) {
        aktifkanBuzzer();
      }
    } else {
      Serial.println("Error mengirim permintaan POST");
    }
    delete[] buffer;
    http.end();
  } else {
    Serial.println("File tidak ada");
  }
}

void hapusRekamanLama() {
  File dir = SD.open("/rekam");
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    if (millis() - entry.getLastWrite() >= intervalHapus) {
      if (entry.isDirectory()) {
        continue;
      }
      Serial.print("Menghapus file: ");
      Serial.println(entry.name());
      SD.remove(entry.name());
    }
    entry.close();
  }
  dir.close();
}

void kirimIDPerangkat() {
  HTTPClient http;
  http.begin(klien, String(serverUrl) + "/register");
  http.addHeader("Content-Type", "application/json");

  String jsonPayload = "{\"device_id\":\"" + String(idPerangkat) + "\"}";
  int kodeResponHttp = http.POST(jsonPayload);

  if (kodeResponHttp > 0) {
    String respon = http.getString();
    Serial.println("Respon server: " + respon);
  } else {
    Serial.println("Error mengirim permintaan POST");
  }

  http.end();
}

void triggerBuzzer(const char* payload) {
  Serial.println("Trigger Buzzer");
  digitalWrite(pinBuzzer, HIGH);
  delay(1000);
  digitalWrite(pinBuzzer, LOW);
}
