#include <Adafruit_GFX.h>       
#include <Adafruit_ST7735.h> 
#include <SPI.h>

#define TFT_CS  15
#define TFT_RST 0
#define TFT_DC  2
#define CTL_INPUT A0 // Whoever thought making only 1 ADC port is a good idea...

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);


// Output voltages respectable for given controls. We only
// care about 2 keypresses at max, and also don't care when
// UP+DOWN or LEFT+RIGHT keys are pressed at the same time.
#define CTL_L  560
#define CTL_LU 842
#define CTL_LD 938
#define CTL_R  357
#define CTL_RU 802
#define CTL_RD 924
#define CTL_U  759
#define CTL_D  908
#define CTL_THRES 6 // <-- Noise tolerance, better keep <10

#define DISPLAY_W 1280
#define DISPLAY_H 1600
#define MAX_FPS 30
#define BGC ST77XX_BLACK


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


InputData processInput() {
    int input = analogRead(A0); // 0 -- 1024
    // ax += ax/4; // Speed acceleration, turned out to be
    // ay += ay/5; // a pretty bad idea for a bullet hell

    if ((input >= CTL_L-CTL_THRES) && (input <= CTL_L+CTL_THRES)) {
        return { 30, 0, 0, 0 };
    }
    if ((input >= CTL_R-CTL_THRES) && (input <= CTL_R+CTL_THRES)) {
        return { 0, 30, 0, 0 };
    }
    if ((input >= CTL_U-CTL_THRES) && (input <= CTL_U+CTL_THRES)) {
        return { 0, 0, 30, 0 };
    }
    if ((input >= CTL_D-CTL_THRES) && (input <= CTL_D+CTL_THRES)) {
        return { 0, 0, 0, 30 };
    }
    if ((input >= CTL_LU-CTL_THRES) && (input <= CTL_LU+CTL_THRES)) {
        return { 17, 0, 17, 0 };
    }
    if ((input >= CTL_LD-CTL_THRES) && (input <= CTL_LD+CTL_THRES)) {
        return { 17, 0, 0, 17 };
    }
    if ((input >= CTL_RU-CTL_THRES) && (input <= CTL_RU+CTL_THRES)) {
        return { 0, 17, 17, 0 };
    }
    if ((input >= CTL_RD-CTL_THRES) && (input <= CTL_RD+CTL_THRES)) {
        return { 0, 17, 0, 17 };
    }

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

    hero.x = (ux + DISPLAY_W + hero.w) % (DISPLAY_W + hero.w);
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
    320,
    160, // 80,
    160, // 80,
    "sdfsdf",
    "none"
};


void loop() {
    InputData inputs = processInput();
    updateHero(hero, inputs);

    Serial.println(
        "L=" + String(inputs.l)
        + ", R=" + String(inputs.r)
        + ", U=" + String(inputs.u)
        + ", D=" + String(inputs.d)
        + ", x=" + String(hero.x)
        + ", y=" + String(hero.y)
    );

    delay(1000 / MAX_FPS);
}
