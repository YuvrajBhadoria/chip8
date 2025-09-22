  #include <bits/types/stack_t.h>
  #include <stdbool.h>
  #include <stdint.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>

  typedef struct {
    uint8_t memory[4096];
    uint8_t V[16];
    uint16_t PC;
    uint16_t I;
    bool drawflag;
    uint16_t stack[16];
    uint16_t sp;
    uint16_t keyboard[16];
    uint8_t soundTimer;
    uint8_t delayTimer;
    uint16_t opcode;
    uint8_t display[32][64];
  } Chip8;

 void execute(Chip8 *c8) {
    uint8_t X, Y, kk, n;
    uint16_t nnn;
    uint32_t i, key_pressed;
  
    /* fetch opcode (two bytes) */
    c8->opcode = (uint16_t)c8->memory[c8->PC] << 8 | c8->memory[c8->PC + 1];
    c8->PC += 2;
    X = (c8->opcode & 0x0F00) >> 8;
    Y = (c8->opcode & 0x00F0) >> 4;
    nnn = (c8->opcode & 0x0FFF);
    kk = (c8->opcode & 0x00FF);
    n = (c8->opcode & 0x000F);

    
    //printf("Opcode: %04x  PC: %04x  I: %04x\n", c8->opcode, c8->PC, c8->I);
  

    switch (c8->opcode & 0xF000) {
    case 0x0000:
      switch (c8->opcode & 0x00FF) {
      case 0x00E0:
        /* 00E0: clear screen */
        memset(c8->display, 0, sizeof(c8->display));
        c8->drawflag = true;
        break;

      case 0x00EE: 
        /* 00EE: return from subroutine */
        if (c8->sp == 0) {
          fprintf(stderr, "Stack underflow on 00EE\n");
        } else {
          --c8->sp;
          c8->PC = c8->stack[c8->sp];
        }
        break;

      default:
        printf("Unknown 0x0 opcode: %04x\n", c8->opcode);
        break;
      }
      break;

    case 0x1000:
      /* 1NNN: jump to address NNN */
      c8->PC = nnn;
      break;

    case 0x2000:
      /* 2NNN: call subroutine at NNN */
      c8->stack[c8->sp] = c8->PC;
      ++c8->sp;
      c8->PC = nnn;
      break;

    case 0x3000:
      /* 3XKK: skip next if VX == KK */
      if (c8->V[X] == kk)
        c8->PC += 2;
      break;

    case 0x4000: 
      /* 4XKK: skip next if VX != KK */
      if (c8->V[X] != kk)
        c8->PC += 2;
      break;

    case 0x5000: 
      /* 5XY0: skip next if VX == VY */
      if (c8->V[X] == c8->V[Y])
        c8->PC += 2;

      break;

    case 0x6000:
      /* 6XKK: VX = KK */
      c8->V[X] = kk;
      break;

    case 0x7000:
      /* 7XKK: VX += KK */
      c8->V[X] += kk;
      break;

    case 0x8000:
      /* 8XYN: arithmetic/bit ops */
      switch (n) {
      case 0x0:
        c8->V[X] = c8->V[Y];
        break;
      case 0x1:
        c8->V[X] |= c8->V[Y];
        break;
      case 0x2:
        c8->V[X] &= c8->V[Y];
        break;
      case 0x3:
        c8->V[X] ^= c8->V[Y];
        break;
      case 0x4: {
        uint16_t sum = (uint16_t)c8->V[X] + (uint16_t)c8->V[Y];
        c8->V[0xF] = (sum > 0xFF) ? 1 : 0;
        c8->V[X] = (uint8_t)(sum & 0xFF);
      } break;
      case 0x5:
        c8->V[0xF] = (c8->V[X] > c8->V[Y]) ? 1 : 0;
        c8->V[X] = c8->V[X] - c8->V[Y];
        break;
      case 0x6:
        c8->V[0xF] = c8->V[X] & 1;
        c8->V[X] >>= 1;
        break;
      case 0x7:
        c8->V[0xF] = (c8->V[Y] > c8->V[X]) ? 1 : 0;
        c8->V[X] = c8->V[Y] - c8->V[X];
        break;
      case 0xE:
        c8->V[0xF] = c8->V[X] >> 7;
        c8->V[X] <<= 1;
        break;
      default:
        printf("Unknown 8XYN opcode: %04x\n", c8->opcode);
        break;
      }
      break;

    case 0x9000: 
      /* 9XY0: skip if VX != VY */
      if (c8->V[X] != c8->V[Y])
        c8->PC += 2;
      break;

    case 0xA000:
      /* ANNN: I = NNN */
      c8->I = nnn;
      break;

    case 0xB000:
      /* BNNN: jump to NNN + V0 */
      c8->PC = nnn + c8->V[0x0];
      break;

    case 0xC000: 
      /* CXKK: VX = random & KK */
      c8->V[X] = (rand() % 256) & kk;
      break;

    case 0xD000: { 
      /* DXYN: draw sprite at (VX, VY), height N */
    
    } break;

    case 0xE000:
      switch (kk) {
      case 0x9E:
       //keyboard instruction
        break;
      case 0xA1:
          //keyboard instruction
        break;
      default:
        printf("Unknown E opcode: %04x\n", c8->opcode);
        break;
      }
      break;

    case 0xF000:
      switch (kk) {
      case 0x07: 
        /* FX07: VX = delay_timer */
        c8->V[X] = c8->delayTimer;
        break;
      case 0x0A: { 
        /* FX0A: wait for a key press, store in VX */
        key_pressed = 0;
        for (i = 0; i < 16; ++i) {
          if (c8->keyboard[i]) {
            key_pressed = 1;
            c8->V[X] = (uint8_t)i;
          }
        }
        if (key_pressed == 0) {
          c8->PC -= 2;
          /* repeat this instruction until key pressed */
        }
      } break;
      case 0x15: 
        /* FX15: delay_timer = VX */
        c8->delayTimer = c8->V[X];
        break;
      case 0x18:
        /* FX18: sound_timer = VX */
        c8->soundTimer = c8->V[X];
        break;
      case 0x1E:
        /* FX1E: I += VX */
        c8->I = c8->I + c8->V[X];
        break;
      case 0x29:              
        /* FX29: I = location of sprite for digit VX */
        c8->I = c8->V[X] * 5; 
        /* each font is 5 bytes */
        break;
      case 0x33: { 
        /* FX33: store BCD of VX at I, I+1, I+2 */
        int vX = c8->V[X];
        c8->memory[c8->I] = (vX / 100) % 10;
        c8->memory[c8->I + 1] = (vX / 10) % 10;
        c8->memory[c8->I + 2] = vX % 10;
      } break;
      case 0x55:
        /* FX55: store V0..VX in memory starting at I */
        for (uint8_t i = 0; i <= X; ++i) {
          c8->memory[c8->I + i] = c8->V[i];
        }
        break;
      case 0x65: 
        /* FX65: read V0..VX from memory starting at I */
        for (uint8_t i = 0; i <= X; ++i) {
          c8->V[i] = c8->memory[c8->I + i];
        }
        break;
      default:
        printf("Unknown F opcode: %04x\n", c8->opcode);
        break;
      }
      break;

    default:
      printf("Unknown opcode: %04x\n", c8->opcode);
      break;
    } /* switch(opcode & 0xF000) Finishes*/
  }


  int main() {

    SDL sdl = {NULL};
    Chip8 c8 = {}; 

    //copying fontset in main memory
    memcpy(c8.memory, chip8_fontset, sizeof(chip8_fontset));



    int scale = 20; // To Scale the window 20 times the chip8 resolution
    c8.PC = 0x200;

    // ROM is loaded
  FILE *fp = fopen("INVADER", "rb");

    if (fp == NULL) {
      fprintf(stderr, "Can't open the ROM file: %s\n", "CUBE8.ch8");
      return 0;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    if (size <= 0 || size > (4096 - 0x200)) {
      fprintf(stderr, "Invalid ROM size: %ld bytes\n", size);
      fclose(fp);
      return 0;
    }
    fseek(fp, 0, SEEK_SET);

    /* Read raw bytes into memory at 0x200 */
    size_t read = fread(c8.memory + 0x200, 1, (size_t)size, fp);
    if (read != (size_t)size) {
      fprintf(stderr, "Failed to read ROM: read %zu of %ld\n", read, size);
      fclose(fp);}
    
    }

    return 0;
      
  }
