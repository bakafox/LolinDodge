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
#define CTL_THRES 6 // <-- Tolerance, keep it below 10!
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
    ST77XX_CYAN, ST77XX_MAGENTA, ST77XX_ORANGE
};
const size_t numColors = sizeof(colors) / sizeof(colors[0]);
unsigned int currColor = ST77XX_YELLOW;


struct Entity {
    short x;
    short y;
    short w;
    short h;
    String color;
    String group;
    String movement;
    short x_prev = 0;
    short y_prev = 0;
    short w_prev = 0;
    short h_prev = 0;
};

struct InputData {
    unsigned char l;
    unsigned char r;
    unsigned char u;
    unsigned char d;
};


InputData processInput(unsigned short& accx, unsigned short& accy) {
    int input = analogRead(A0); // 0 -- 1024

    accx = min(accx + accx/3, 180);
    accy = min(accy + accy/4, 120);

    if ((input >= CTL_L-CTL_THRES) && (input <= CTL_L+CTL_THRES)) {
        return { 20+(accx/3), 0, 0, 0 };
    }
    if ((input >= CTL_R-CTL_THRES) && (input <= CTL_R+CTL_THRES)) {
        return { 0, 20+(accx/3), 0, 0 };
    }
    if ((input >= CTL_U-CTL_THRES) && (input <= CTL_U+CTL_THRES)) {
        return { 0, 0, 20+(accy/4), 0 };
    }
    if ((input >= CTL_D-CTL_THRES) && (input <= CTL_D+CTL_THRES)) {
        return { 0, 0, 0, 20+(accy/4) };
    }
    if ((input >= CTL_LU-CTL_THRES) && (input <= CTL_LU+CTL_THRES)) {
        return { 14+(accx/3), 0, 14+(accy/4), 0 };
    }
    if ((input >= CTL_LD-CTL_THRES) && (input <= CTL_LD+CTL_THRES)) {
        return { 14+(accx/3), 0, 0, 14+(accy/4) };
    }
    if ((input >= CTL_RU-CTL_THRES) && (input <= CTL_RU+CTL_THRES)) {
        return { 0, 14+(accx/3), 14+(accy/4), 0 };
    }
    if ((input >= CTL_RD-CTL_THRES) && (input <= CTL_RD+CTL_THRES)) {
        return { 0, 14+(accx/3), 0, 14+(accy/4) };
    }

    // Don't change to init instantly in case of short keys release
    accx = max(3, accx/2);
    accy = max(4, accy/2);

    return { 0, 0, 0, 0 };
}


void redrawEntity(Entity& e) {
    short dx = e.x - e.x_prev;
    short dy = e.y - e.y_prev;
    short dw = e.w_prev - e.w;
    short dh = e.h_prev - e.h;

    if (dx != 0) {
        if (dx > 0) {
            tft.fillRect(
                (e.x_prev)/10-1, (e.y_prev)/10-1, (dx+9)/10+1, (e.h_prev+9)/10+1,
            BGC);
        }
        else {
            tft.fillRect(
                (e.x+e.w+9)/10-1, (e.y_prev+9)/10-1, (-dx)/10+1, (e.h_prev)/10+1,
            BGC);
        }
    }
    if (dy != 0) {
        if (dy > 0) {
            tft.fillRect(
                (e.x_prev)/10-1, (e.y_prev)/10-1, (e.w_prev+9)/10+1, (dy+9)/10+1,
            BGC);
        }
        else {
            tft.fillRect(
                (e.x_prev+9)/10-1, (e.y+e.w+9)/10-1, (e.w_prev)/10+1, (-dy)/10+1,
            BGC);
        }
    }
    if (dw > 0) {
        tft.fillRect(
            (e.x+e.w)/10-1, (e.y_prev)/10-1, (dw)/10+1, (e.h_prev+9)/10+1,
        BGC);
    }
    if (dh > 0) {
        tft.fillRect(
            (e.x_prev)/10-1, (e.y+e.h)/10-1, (e.w_prev+9)/10+1, (dh)/10+1,
        BGC);
    }

    tft.fillRect(
        e.x / 10, e.y / 10, e.w / 10, e.h / 10,
    currColor);
    currColor = colors[random(numColors)];

    e.x_prev = e.x;
    e.y_prev = e.y;
    e.w_prev = e.w;
    e.h_prev = e.h;
}


void updateHero(Entity& hero, InputData inputs) {
    short ux = (hero.x + inputs.l - inputs.r);
    short uy = (hero.y + inputs.u - inputs.d);

    hero.x = (ux + hero.w/2 + DISPLAY_W) % DISPLAY_W - hero.w/2;
    if (uy > 0 && uy < DISPLAY_H - hero.h) {
        hero.y = uy;
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
    DISPLAY_W/2 - 80,
    DISPLAY_H - 320,
    80,
    80,
    "sdfsdf",
    "none"
};

// TODO: move out acceleration logic to updateHero() so it
// won't accumulate when hero is touching walls or collisions.
unsigned short accx = 3;
unsigned short accy = 4;

void loop() {
    InputData inputs = processInput(accx, accy);
    updateHero(hero, inputs);

    Serial.println(
        "L=" + String(inputs.l)
        + ", R=" + String(inputs.r)
        + ", U=" + String(inputs.u)
        + ", D=" + String(inputs.d)
        + ", x=" + String(hero.x)
        + ", y=" + String(hero.y)
        + ", x_a=" + String(accx)
        + ", y_a=" + String(accy)
    );

    delay(1000 / MAX_FPS);
}
