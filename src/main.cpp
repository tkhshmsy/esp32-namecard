#include <M5EPD.h>
#include <ArduinoJson.h>

M5EPD_Canvas canvas(&M5.EPD);
char json[10240];
JsonDocument doc;

#define BTN_UP  GPIO_NUM_37
#define BTN_GO  GPIO_NUM_38
#define BTN_DOWN GPIO_NUM_39

int SleepCounter = 0;
int MaxPages = 10;
RTC_DATA_ATTR int PageCount = 0;

void deepSleep() {
    Serial.println("Deep Sleep");
    esp_sleep_enable_ext0_wakeup(BTN_GO, LOW);
    esp_deep_sleep_start();
}

void readJson() {
    Serial.println("Read JSON");
    File file = SD.open("/namecard/namecard.json");
    if (!file) {
        canvas.setTextSize(3);
        canvas.drawString("File not found", 45, 650);
        canvas.pushCanvas(0,0,UPDATE_MODE_GC16);
        M5.update();
        deepSleep();
    }
    file.readBytes(json, file.size());
    file.close();

    DeserializationError error = deserializeJson(doc, json);
    serializeJsonPretty(doc, Serial);
    JsonArray cards = doc["cards"].as<JsonArray>();
    MaxPages = (int)cards.size();
}

void drawCard() {
    JsonArray cards = doc["cards"].as<JsonArray>();
    const char *logo = cards[PageCount]["logo"];
    const char *name = cards[PageCount]["name"];
    const char *url = cards[PageCount]["url"];

    Serial.println("Draw Card");
    Serial.println(PageCount);
    Serial.println(logo);
    Serial.println(name);
    Serial.println(url);

    canvas.clear();
    canvas.drawPngFile(SD, logo, 30, 0);
    canvas.drawString(name, 30, 485);
    canvas.qrcode(url, 30 + 30, 540, 420);
    canvas.pushCanvas(0,0,UPDATE_MODE_GC16);
}

void setup() {
    M5.begin();
    M5.EPD.SetRotation(90);
    M5.EPD.Clear(true);
    M5.RTC.begin();
    Serial.begin(115200);
    SD.begin();

    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_GO, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);

    canvas.createCanvas(540, 960);
    canvas.setTextSize(3);

    readJson();
    PageCount %= MaxPages;
    drawCard();
}

void loop() {
    PageCount %= MaxPages;
    int cur = PageCount;

    bool update = false;
    if (digitalRead(BTN_UP) == LOW) {
        cur++;
        update = true;
    }
    if (digitalRead(BTN_DOWN) == LOW) {
        cur--;
        if (cur < 0) {
            cur = MaxPages - 1;
        }
        update = true;
    }

    cur %= MaxPages;
    if (update) {
        PageCount = cur;
        drawCard();
        M5.update();
        SleepCounter = 0;
        update = false;
    }

    SleepCounter++;
    delay(100);
    if (SleepCounter >= 600) {
        deepSleep();
    }
}
