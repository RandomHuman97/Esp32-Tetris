//the basic structure
// Game class because we dont need alot of classes
// we can just use the game class to handle everything
// piece will check collision every move
// and if it touches the ground it will place it in the grid

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EEPROM.h>
#define BUTTON_4 12
#define BUTTON_5 14
#define BUTTON_6 27
#define BUTTON_1 26
#define BUTTON_2 25
#define BUTTON_3 23

#define INITIAL_PIECE_X  2
#define INITIAL_PIECE_Y 0
#define GRID_WIDTH 8
#define GRID_HEIGHT 15
#define ACTION_TIMEOUT 5
#define PIECE_TIMEOUT 20
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R1, /* reset=*/U8X8_PIN_NONE);

//tile data
const uint8_t tile_block[8] = {
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
};
const uint8_t tile_block_squared[8] = {
    0b11111111,
    0b10000001,
    0b10111101,
    0b10111101,
    0b10111101,
    0b10111101,
    0b10000001,
    0b11111111,
};
const uint8_t tile_block_holepunched[8] = {
    0b11111111,
    0b10000001,
    0b10111101,
    0b10100101,
    0b10100101,
    0b10111101,
    0b10000001,
    0b11111111,
};
/*
0,0         60,0






0,120       60,120
*/

byte piece_square[3][3] = {
    {1, 1, 1},
    {1, 1, 1},
    {1, 1, 1}};
    
byte piece_l[3][3] = {
    {0, 0, 0},
    {1, 1, 1},
    {1, 0, 0}};
    
byte piece_t[3][3] = {
    {0, 0, 0},
    {1, 1, 1},
    {0, 1, 0}};
    
byte piece_z[3][3] = {
    {0, 0, 0},
    {1, 1, 0},
    {0, 1, 1}};
byte piece_i[3][3] = {
    {0, 0, 0},
    {1, 1, 1},
    {0, 0, 0}};
byte (*pieces[])[3][3] = {
    &piece_l,
    &piece_t,
    &piece_z,
    &piece_i,
    &piece_square};
unsigned long score = 0;
String message = "";
byte game_state = 1;

void EEPROM_writeLong(int address, unsigned long value) {
    Serial.printf("Writing value to EEPROM: %lu\n", value);
    
    byte value_array[4];
    for (int i = 0; i < 4; i++) {
        value_array[i] = (value >> (i * 8)) & 0xFF;
        Serial.printf("Byte %d: 0x%02X\n", i, value_array[i]);
    }
    
    for (int i = 0; i < 4; i++) {
        EEPROM.write(address + i, value_array[i]);
    }
    
    if (EEPROM.commit()) {
        Serial.println("EEPROM write successful");
    } else {
        Serial.println("EEPROM write failed!");
    }
}

unsigned long EEPROM_readLong(int address) {
    byte value_array[4];
    for (int i = 0; i < 4; i++) {
        value_array[i] = EEPROM.read(address + i);
    }
    unsigned long value = 0;
    for (int i = 0; i < 4; i++) {
        value |= ((unsigned long)value_array[i]) << (i * 8);
    }
    return value;
}
class Game {
    public:
        byte grid[GRID_WIDTH][GRID_HEIGHT];
        byte current_piece[3][3];
        int current_piece_x;
        int current_piece_y;
        int current_piece_type;
        int current_piece_skin;
        int current_piece_rotation;
        bool new_piece = true;

        Game() {
            // Initialize the grid
            for (int i = 0; i < GRID_WIDTH; i++) {
                for (int j = 0; j < GRID_HEIGHT; j++) {
                    grid[i][j] = 0;
                }
            }

            current_piece_x = INITIAL_PIECE_X;
            current_piece_y = INITIAL_PIECE_Y;
            current_piece_type = 0;
            current_piece_rotation = 0;
            current_piece_skin = 1;
            // Copy the piece to the current piece
            memcpy(current_piece, pieces[current_piece_type], sizeof(current_piece));
        }
        void stampPiece() {
            new_piece = true;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (current_piece[i][j] == 1) {
                        grid[current_piece_x + i][current_piece_y + j] = current_piece_skin;
                    }
                }
            }
            current_piece_x = INITIAL_PIECE_X;
            current_piece_y = INITIAL_PIECE_Y;
            current_piece_type = random(0, 5);
            current_piece_rotation = 0;
            current_piece_skin = current_piece_skin % 3 + 1;
            memcpy(current_piece, pieces[current_piece_type], sizeof(current_piece));
            delay(250);
        }

        void movePiece(int x, int y) {
            int new_x = current_piece_x + x;
            int new_y = current_piece_y + y;
            int collision = checkCollision(new_x, new_y, current_piece_rotation);
            if (collision==2) {
                if (new_piece) {
                        game_state = 0;
                }
                stampPiece();
                return;
            }

            if (collision==1) {
                if (y == 1 && x == 0) {
                    if (new_piece) {
                        game_state = 0;
                    }
                    //hacky ahhhh
                    stampPiece();
                    return;

                }
                return;
            }
            new_piece = false;
            current_piece_y = new_y;
            if (collision == 3) {
                return;
            }
            current_piece_x = new_x;
        }

        int checkCollision(int x, int y, int rotation){
            // returns 0 if no collision, 1 if standard collision, 2 if bottom collision
            byte piece[3][3];
            // Copy the piece to the current piece
            memcpy(piece, pieces[current_piece_type], sizeof(piece));
            // Rotate the piece
            for (int i = 0; i < rotation; i++) {
                rotatePiece(piece);
            }

            // Check if the piece collides with the grid
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (piece[i][j] == 1) {
                        if (grid[x + i][y + j] != 0) {
                            return 1;
                        }
                        //here we check if the piece is at the bottom
                        if (y + j == GRID_HEIGHT) { 
                            return 2;
                        }
                        //check if colliding with sides 
                        if (x + i < 0|| x + i == GRID_WIDTH) {
                            return 3;
                        }
                    }
                }
            }
            return 0;
            
        }
        void rotatePiece(byte piece[3][3]) {
            byte new_piece[3][3];
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    new_piece[i][j] = piece[j][2 - i];
                }
            }
            memcpy(piece, new_piece, sizeof(new_piece));
        }
        void safeRotatePiece(byte piece[3][3]) {
            byte new_piece[3][3];
            memcpy(new_piece, piece, sizeof(new_piece));
            rotatePiece(new_piece);
            if (checkCollision(current_piece_x, current_piece_y, (current_piece_rotation + 1 % 4)) == 0) {
                memcpy(piece, new_piece, sizeof(new_piece));
                current_piece_rotation = (current_piece_rotation + 1) % 4;
                
            } 
        }
        void render(){
            //rendering the grid
            for (int i = 0; i < GRID_WIDTH; i++) {
                for (int j = 0; j < GRID_HEIGHT; j++) {
                    if (grid[i][j] == 1) {
                        //u8g2.drawBox(i * 8, j * 8, 8, 8);
                        u8g2.drawXBMP(i * 8, j * 8, 8, 8 , tile_block); 
                    }
                    if (grid[i][j] == 2){
                        //u8g2.drawFrame(i * 8, j * 8, 8, 8);
                        // draw 2x2 box inside the frame
                        //u8g2.drawFrame(i * 8 + 2, j * 8 + 2, 4, 4);
                        u8g2.drawXBMP(i * 8, j * 8, 8, 8 , tile_block_holepunched); 
                    }
                    if (grid[i][j] == 3){
                        //u8g2.drawFrame(i * 8, j * 8, 8, 8);
                        // draw 2x2 box inside the frame
                        //u8g2.drawBox(i * 8 + 2, j * 8 + 2, 4, 4);
                        u8g2.drawXBMP(i * 8, j * 8, 8, 8 , tile_block_squared); 
                    }
                }
            }
            // render ghost piece
            int ghost_y = current_piece_y;
            while (checkCollision(current_piece_x, ghost_y, current_piece_rotation) == 0) {
                ghost_y += 1;
            }
            ghost_y -= 1;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (current_piece[i][j] == 1) {
                        u8g2.drawFrame((current_piece_x + i) * 8, (ghost_y + j) * 8, 8, 8);
                    }
                }
            }

            //rendering the current piece
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (current_piece[i][j] == 1) {
                        if (current_piece_skin == 1) {
                            //u8g2.drawBox((current_piece_x + i) * 8, (current_piece_y + j) * 8, 8, 8);
                            u8g2.drawXBMP((current_piece_x + i) * 8, (current_piece_y + j) * 8, 8, 8 , tile_block);
                        } else if (current_piece_skin == 2) {
                            u8g2.drawXBMP((current_piece_x + i) * 8, (current_piece_y + j) * 8, 8, 8 , tile_block_holepunched);
                            //u8g2.drawFrame((current_piece_x + i) * 8, (current_piece_y + j) * 8, 8, 8);
                            //u8g2.drawFrame((current_piece_x + i) * 8 + 2, (current_piece_y + j) * 8 + 2, 4, 4);
                        } else if (current_piece_skin == 3) {

                            u8g2.drawXBMP((current_piece_x + i) * 8, (current_piece_y + j) * 8, 8, 8 , tile_block_squared);
                            //u8g2.drawFrame((current_piece_x + i) * 8, (current_piece_y + j) * 8, 8, 8);
                            //drawBox((current_piece_x + i) * 8 + 2, (current_piece_y + j) * 8 + 2, 4, 4);
                        }
                    }
                }
            }
            
            
        }
        void clearRows(){
            byte rows_combo = 0;
            for (int j = GRID_HEIGHT - 1; j >= 0; j--) {
            byte row_filled = 1;
            for (int i = 0; i < GRID_WIDTH; i++) {
                if (grid[i][j] == 0) {
                row_filled = 0;
                break;
                }
            }
            if (row_filled == 1) {
                rows_combo += 1;
                for (int k = j; k > 0; k--) {
                for (int i = 0; i < GRID_WIDTH; i++) {
                    grid[i][k] = grid[i][k - 1];
                }
                }
                for (int i = 0; i < GRID_WIDTH; i++) {
                grid[i][0] = 0;
                }
                j += 1;
            }
            }
            if (rows_combo == 1) {
                score += 40;
                message = "Single";
            } else if( rows_combo == 2) {
                score += 100;
                message = "Double!";
            } else if (rows_combo == 3) {
                score += 300;
                message = "Triple!";
            } else if (rows_combo >= 4) {
                score += 1200;
                message = "Tetris!!!";
            }   

        }


};
Game game;

void setup() {
    Wire.begin();
    Wire.setClock(400000); //la overclock =)
    EEPROM.begin(512);
    Serial.begin(115200);
    //randomize the random number generator
    randomSeed(esp_random());

    u8g2.begin();
    u8g2.setFont(u8g2_font_squeezed_b6_tr);
    pinMode(BUTTON_1, INPUT_PULLUP);
    pinMode(BUTTON_2, INPUT_PULLUP);
    pinMode(BUTTON_3, INPUT_PULLUP);
    pinMode(BUTTON_4, INPUT_PULLUP);
    pinMode(BUTTON_5, INPUT_PULLUP);
    pinMode(BUTTON_6, INPUT_PULLUP);
    u8g2.clearBuffer();
    if (digitalRead(BUTTON_4)== LOW) {
        EEPROM_writeLong(0, 0);
    }

}
byte tick = 0;
byte actionTick = 0;
void loop() {
    while (game_state == 1){
    tick += 1;
    if (tick == PIECE_TIMEOUT) {
        game.movePiece(0, 1);
        tick = 0;
        score += 1;
    }

    if (digitalRead(BUTTON_1) == LOW && actionTick == 0) {
        game.movePiece(-1, 0);
        actionTick= ACTION_TIMEOUT;
    }
    if (digitalRead(BUTTON_2) == LOW) {
        game.movePiece(0, 1);
        score += 2;
    }
    if (digitalRead(BUTTON_3) == LOW && actionTick == 0) {
        game.movePiece(1, 0);
        actionTick = ACTION_TIMEOUT;
    } 
    if (digitalRead(BUTTON_5)== LOW && actionTick == 0) {
        game.safeRotatePiece(game.current_piece);
        actionTick = ACTION_TIMEOUT;
    }
    if (actionTick > 0) {
        actionTick -= 1;
    }
    game.clearRows();
    u8g2.clearBuffer();
    game.render();
    u8g2.setCursor(0, 128);
    u8g2.print(score);
    u8g2.print(" ");
    u8g2.print(message);
    u8g2.sendBuffer();
    }
    if (EEPROM_readLong(0) < score) {
        EEPROM_writeLong(0, score);
    }
    u8g2.clearBuffer();
    u8g2.setCursor(0, 8);
    u8g2.print("Game Over!");
    u8g2.setCursor(0, 64);
    u8g2.print("High Score: ");
    u8g2.setCursor(0, 72);
    u8g2.print(EEPROM_readLong(0));
    u8g2.setCursor(0, 128);
    u8g2.print("Score: ");
    u8g2.print(score);

    u8g2.sendBuffer();

    while (true){
        if (digitalRead(BUTTON_2) == LOW || digitalRead(BUTTON_3) == LOW || digitalRead(BUTTON_1) == LOW) {
            ESP.restart();
        }
    }
}
