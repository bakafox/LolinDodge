#include <Adafruit_GFX.h>       
#include <Adafruit_ST7735.h> 
#include <SPI.h>

#define TFT_CS  15
#define TFT_RST 0
#define TFT_DC  2
#define CTL_INPUT A0 // Whoever thought making only 1 ADC port is a good idea...

// Output voltages respectable for given controls. We only
// care about 2 keypresses at max, and also don't care when
// UP+DOWN or LEFT+RIGHT keys are pressed at the same time.
#define CTL_THRES 6 // <-- +/- tolerance, keep below 10!
#define CTL_L  560
#define CTL_LU 842
#define CTL_LD 938
#define CTL_R  357
#define CTL_RU 802
#define CTL_RD 924
#define CTL_U  759
#define CTL_D  908

#define DISPLAY_W 1280
#define DISPLAY_H 1600
#define MAX_FPS 30 // Not like esp8266 can perform any faster anyway...
#define BGC ST77XX_BLACK

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);


const uint16_t colors[] = {
    ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE, ST77XX_WHITE,
    ST77XX_YELLOW, ST77XX_CYAN, ST77XX_MAGENTA, ST77XX_ORANGE
};
const size_t numColors = sizeof(colors) / sizeof(colors[0]);
unsigned int currColor = ST77XX_YELLOW;


struct PosData {
    short x;
    short y;
    short x_prev = 0;
    short y_prev = 0;
};

struct MovData {
    signed char lr; // Negative = left, Positive = right
    signed char ud; // Negative = up, Positive = down
    unsigned char lr_a = 0;
    unsigned char ud_a = 0;
};

struct Entity {
    PosData pos;
    MovData mov;
    char w;
    char h;
    String type;
};

struct Bullet {
    PosData pos;
    MovData mov;
    char s;
    bool isRepulsive;
};


MovData processInput(MovData prevInput) {
    int input = analogRead(A0); // 0 -- 1024

    unsigned char lr_a = min(
        abs(prevInput.lr)/7 + prevInput.lr_a*4/3 + prevInput.ud_a/21,
    180);
    unsigned char ud_a = min(
        abs(prevInput.ud)/7 + prevInput.ud_a*4/3 + prevInput.lr_a/21,
    180);

    if ((input >= CTL_L-CTL_THRES) && (input <= CTL_L+CTL_THRES)) {
        return MovData { 20 + lr_a/3, 0, lr_a, ud_a/2 };
    }
    if ((input >= CTL_R-CTL_THRES) && (input <= CTL_R+CTL_THRES)) {
        return MovData { -20 - lr_a/3, 0, lr_a, ud_a/2 };
    }
    if ((input >= CTL_U-CTL_THRES) && (input <= CTL_U+CTL_THRES)) {
        return MovData { 0, 20 + ud_a/3, lr_a/2, ud_a };
    }
    if ((input >= CTL_D-CTL_THRES) && (input <= CTL_D+CTL_THRES)) {
        return MovData { 0, -20 - ud_a/3, lr_a/2, ud_a };
    }
    if ((input >= CTL_LU-CTL_THRES) && (input <= CTL_LU+CTL_THRES)) {
        return MovData { 14 + lr_a/4, 14 + ud_a/4, lr_a, ud_a };
    }
    if ((input >= CTL_LD-CTL_THRES) && (input <= CTL_LD+CTL_THRES)) {
        return MovData { 14 + lr_a/4, -14 - ud_a/4, lr_a, ud_a };
    }
    if ((input >= CTL_RU-CTL_THRES) && (input <= CTL_RU+CTL_THRES)) {
        return MovData { -14 - lr_a/4, 14 + ud_a/4, lr_a, ud_a };
    }
    if ((input >= CTL_RD-CTL_THRES) && (input <= CTL_RD+CTL_THRES)) {
        return MovData { -14 - lr_a/4, -14 - ud_a/4, lr_a, ud_a };
    }

    return MovData { prevInput.lr/3, prevInput.ud/3, lr_a/2, ud_a/2 };
}


void redrawEntity(Entity& e) {
    short dx = e.pos.x - e.pos.x_prev;
    short dy = e.pos.y - e.pos.y_prev;

    // if (e.type == "hero") {
    //     tft.fillTriangle(
    //         e.pos.x/10 - 2, e.pos.y/10 - 2,
    //         e.pos.x/10 + e.w/2/10, e.pos.y/10 + e.h/10 + 2,
    //         e.pos.x/10 + e.w/10 + 2, e.pos.y/10 - 2,
    //     BGC);
    //     tft.fillTriangle(
    //         e.pos.x/10, e.pos.y/10,
    //         e.pos.x/10 + e.w/2/10, e.pos.y/10 + e.h/10,
    //         e.pos.x/10 + e.w/10, e.pos.y/10,
    //     ST77XX_ORANGE);
    // }

    if (dx != 0) {
        if (dx > 0) {
            tft.fillRect(
                (e.pos.x_prev)/10-1, (e.pos.y_prev)/10-1, (dx+9)/10+1, (e.h+9)/10+1,
            BGC);
        }
        else {
            tft.fillRect(
                (e.pos.x+e.w+9)/10-1, (e.pos.y_prev+9)/10-1, (-dx)/10+1, (e.h)/10+1,
            BGC);
        }
    }
    if (dy != 0) {
        if (dy > 0) {
            tft.fillRect(
                (e.pos.x_prev)/10-1, (e.pos.y_prev)/10-1, (e.w+9)/10+1, (dy+9)/10+1,
            BGC);
        }
        else {
            tft.fillRect(
                (e.pos.x_prev+9)/10-1, (e.pos.y+e.w+9)/10-1, (e.w)/10+1, (-dy)/10+1,
            BGC);
        }
    }

    tft.fillRect(
        e.pos.x / 10, e.pos.y / 10, e.w / 10, e.h / 10,
    currColor);

    currColor = colors[random(numColors)];

    e.pos.x_prev = e.pos.x;
    e.pos.y_prev = e.pos.y;
}


void heroUpdatePos(Entity& hero) {
    short ux = (hero.pos.x + hero.mov.lr);
    short uy = (hero.pos.y + hero.mov.ud);

    hero.pos.x = (ux + hero.w/2 + DISPLAY_W) % DISPLAY_W - hero.w/2;

    if (uy > 0 && uy < DISPLAY_H - hero.h) {
        hero.pos.y = uy;
    }

    redrawEntity(hero);
}


void setup() {
    Serial.begin(9600);

    tft.initR(INITR_BLACKTAB);
    tft.setRotation(0); // Set display to vertical
    tft.fillScreen(BGC);
}


Entity hero = {
    { DISPLAY_W/2 - 80, DISPLAY_H - 320 },
    { 0, 0 },
    160, // 80 is too easy
    160, // 80 is too easy
    "hero"
};

Entity enemies[20];

void loop() {
    hero.mov = processInput(hero.mov);
    heroUpdatePos(hero);

    Serial.println(
        "LR=" + String(hero.mov.lr)
        + ", UD=" + String(hero.mov.ud)
        + ", LRA=" + String(hero.mov.lr_a)
        + ", UDA=" + String(hero.mov.ud_a)
        + ", x=" + String(hero.pos.x)
        + ", y=" + String(hero.pos.y)
    );

    delay(1000 / MAX_FPS);
}
