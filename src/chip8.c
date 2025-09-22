#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <bits/types/stack_t.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
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

typedef struct {

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;

} SDL;

static const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10,
    0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10,
    0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0,
    0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,
    0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80,
    0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80};

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

  // printf("Opcode: %04x  PC: %04x  I: %04x\n", c8->opcode, c8->PC, c8->I);

  switch (c8->opcode & 0xF000) {
  case 0x0000:
    switch (c8->opcode & 0x00FF) {
    case 0x00E0: /* 00E0: clear screen */
      memset(c8->display, 0, sizeof(c8->display));
      c8->drawflag = true;
      break;

    case 0x00EE: /* 00EE: return from subroutine */
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

  case 0x1000: /* 1NNN: jump to address NNN */
    c8->PC = nnn;
    break;

  case 0x2000: /* 2NNN: call subroutine at NNN */
    c8->stack[c8->sp] = c8->PC;
    ++c8->sp;
    c8->PC = nnn;
    break;

  case 0x3000: /* 3XKK: skip next if VX == KK */
    if (c8->V[X] == kk)
      c8->PC += 2;
    break;

  case 0x4000: /* 4XKK: skip next if VX != KK */
    if (c8->V[X] != kk)
      c8->PC += 2;
    break;

  case 0x5000: /* 5XY0: skip next if VX == VY */
    if (c8->V[X] == c8->V[Y])
      c8->PC += 2;

    break;

  case 0x6000: /* 6XKK: VX = KK */
    c8->V[X] = kk;
    break;

  case 0x7000: /* 7XKK: VX += KK */
    c8->V[X] += kk;
    break;

  case 0x8000: /* 8XYN: arithmetic/bit ops */
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

  case 0x9000: /* 9XY0: skip if VX != VY */
    if (c8->V[X] != c8->V[Y])
      c8->PC += 2;
    break;

  case 0xA000: /* ANNN: I = NNN */
    c8->I = nnn;
    printf("FX29 %d", c8->I);
    break;

  case 0xB000: /* BNNN: jump to NNN + V0 */
    c8->PC = nnn + c8->V[0x0];
    break;

  case 0xC000: /* CXKK: VX = random & KK */
    c8->V[X] = (rand() % 256) & kk;
    break;

  case 0xD000: { /* DXYN: draw sprite at (VX, VY), height N */
    uint16_t px = c8->V[X];
    uint16_t py = c8->V[Y];
    uint16_t numberOfSprites = n;
    uint8_t spriteByte;
    c8->V[0xF] = 0;

    for (uint16_t spriteNumber = 0; spriteNumber < numberOfSprites;
         ++spriteNumber) {
      spriteByte = c8->memory[c8->I + spriteNumber];
      for (uint8_t bitNumberInSprite = 0; bitNumberInSprite < 8;
           ++bitNumberInSprite) {

        uint8_t spritePixel = (spriteByte >> (7 - bitNumberInSprite)) & 1;

        if (spritePixel) {
          uint16_t X = (px + bitNumberInSprite) % 64;
          uint16_t Y = (py + spriteNumber) % 32;

          if (c8->display[Y][X] == 1) {
            c8->V[0xF] = 1;
          }
          c8->display[Y][X] ^= 1;
        }
      }
    }
    c8->drawflag = true;
  } break;

  case 0xE000:
    switch (kk) {
    case 0x9E:
      if (c8->keyboard[c8->V[X]] != 0)
        c8->PC += 2;
      break;
    case 0xA1:
      if (c8->keyboard[c8->V[X]] == 0)
        c8->PC += 2;
      break;
    default:
      printf("Unknown E opcode: %04x\n", c8->opcode);
      break;
    }
    break;

  case 0xF000:
    switch (kk) {
    case 0x07: /* FX07: VX = delay_timer */
      c8->V[X] = c8->delayTimer;
      break;
    case 0x0A: { /* FX0A: wait for a key press, store in VX */
      key_pressed = 0;
      for (i = 0; i < 16; ++i) {
        if (c8->keyboard[i]) {
          key_pressed = 1;
          c8->V[X] = (uint8_t)i;
        }
      }
      if (key_pressed == 0) {
        c8->PC -= 2; /* repeat this instruction until key pressed */
      }
    } break;
    case 0x15: /* FX15: delay_timer = VX */
      c8->delayTimer = c8->V[X];
      break;
    case 0x18: /* FX18: sound_timer = VX */
      c8->soundTimer = c8->V[X];
      break;
    case 0x1E: /* FX1E: I += VX */
      c8->I = c8->I + c8->V[X];
      break;
    case 0x29:
      /* FX29: I = location of sprite for digit VX */
      c8->I = c8->V[X] * 5; /* each font is 5 bytes */
      break;
    case 0x33: { /* FX33: store BCD of VX at I, I+1, I+2 */
      int vX = c8->V[X];
      c8->memory[c8->I] = (vX / 100) % 10;
      c8->memory[c8->I + 1] = (vX / 10) % 10;
      c8->memory[c8->I + 2] = vX % 10;
    } break;
    case 0x55: /* FX55: store V0..VX in memory starting at I */
      for (uint8_t i = 0; i <= X; ++i) {
        c8->memory[c8->I + i] = c8->V[i];
      }
      break;
    case 0x65: /* FX65: read V0..VX from memory starting at I */
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

void draw(SDL *sdl, Chip8 *c8) {
  // Rendering the Color of Background !!

  if (c8->drawflag) {
    SDL_SetRenderDrawColor(sdl->renderer, 0, 0, 0, 255);
    SDL_RenderClear(sdl->renderer);
    uint32_t drawPixel[32][64];
    memset(drawPixel, 0, sizeof(drawPixel));

    for (int py = 0; py < 32; ++py) {

      for (int px = 0; px < 64; ++px) {

        if (c8->display[py][px] == 1) {

          drawPixel[py][px] = 0xFFFFFFFFu;
        }
      }
    }

    SDL_UpdateTexture(sdl->texture, NULL, drawPixel, 64 * sizeof(uint32_t));

    SDL_FRect position;
    position.x = 0;
    position.y = 0;
    position.w = 64;
    position.h = 32;
    SDL_RenderTexture(sdl->renderer, sdl->texture, NULL, &position);
    SDL_RenderPresent(sdl->renderer);
  }
  c8->drawflag = false;
}

void handle_key_event(SDL_Event *ev, Chip8 *c8, int32_t *delay) {

  if (ev->type == SDL_EVENT_KEY_DOWN) {

    switch (ev->key.key) {

    case SDLK_F3:
      *delay += 1;
      break;
    case SDLK_F2:
      *delay -= 1;
      break;
    case SDLK_1:
      c8->keyboard[1] = 1;
      break;
    case SDLK_2:
      c8->keyboard[2] = 1;
      break;
    case SDLK_3:
      c8->keyboard[3] = 1;
      break;
    case SDLK_Q:
      c8->keyboard[4] = 1;
      break;
    case SDLK_W:
      c8->keyboard[5] = 1;
      break;
    case SDLK_E:
      c8->keyboard[6] = 1;
      break;
    case SDLK_X:
      c8->keyboard[0] = 1;
      break;
    case SDLK_A:
      c8->keyboard[7] = 1;
      break;
    case SDLK_S:
      c8->keyboard[8] = 1;
      break;
    case SDLK_D:
      c8->keyboard[9] = 1;
      break;
    case SDLK_Z:
      c8->keyboard[0xA] = 1;
      break;
    case SDLK_C:
      c8->keyboard[0xB] = 1;
      break;
    case SDLK_4:
      c8->keyboard[0xC] = 1;
      break;
    case SDLK_R:
      c8->keyboard[0xD] = 1;
      break;
    case SDLK_F:
      c8->keyboard[0xE] = 1;
      break;
    case SDLK_V:
      c8->keyboard[0xF] = 1;
      break;
    }

  }

  else if (ev->type == SDL_EVENT_KEY_UP) {
    switch (ev->key.key) {
    case SDLK_X:
      c8->keyboard[0] = 0;
      break;
    case SDLK_1:
      c8->keyboard[1] = 0;
      break;
    case SDLK_2:
      c8->keyboard[2] = 0;
      break;
    case SDLK_3:
      c8->keyboard[3] = 0;
      break;
    case SDLK_Q:
      c8->keyboard[4] = 0;
      break;
    case SDLK_W:
      c8->keyboard[5] = 0;
      break;
    case SDLK_E:
      c8->keyboard[6] = 0;
      break;
    case SDLK_A:
      c8->keyboard[7] = 0;
      break;
    case SDLK_S:
      c8->keyboard[8] = 0;
      break;
    case SDLK_D:
      c8->keyboard[9] = 0;
      break;
    case SDLK_Z:
      c8->keyboard[0xA] = 0;
      break;
    case SDLK_C:
      c8->keyboard[0xB] = 0;
      break;
    case SDLK_4:
      c8->keyboard[0xC] = 0;
      break;
    case SDLK_R:
      c8->keyboard[0xD] = 0;
      break;
    case SDLK_F:
      c8->keyboard[0xE] = 0;
      break;
    case SDLK_V:
      c8->keyboard[0xF] = 0;
      break;
    }
  }
}
bool loadrom(Chip8 *c8, const char *rompath) {
  FILE *fp = fopen(rompath, "rb");

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
  size_t read = fread(c8->memory + 0x200, 1, (size_t)size, fp);
  if (read != (size_t)size) {
    fprintf(stderr, "Failed to read ROM: read %zu of %ld\n", read, size);
    fclose(fp);
  }

  return true;
}

int main(int argc, char **argv) {

  SDL sdl = {NULL};
  Chip8 c8 = {};

  // copying fontset in main memory
  memcpy(c8.memory, chip8_fontset, sizeof(chip8_fontset));

  // loading Rom
  if (!loadrom(&c8, argv[1])) {

    return EXIT_FAILURE;
  }

  int scale = 10; // To Scale window according to the user
  c8.PC = 0x200;
  SDL_Init(SDL_INIT_VIDEO);

  sdl.window = SDL_CreateWindow("Chip8-SDL3", 64 * scale, 32 * scale, 0);
  if (!sdl.window) {
    fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());

    SDL_Quit();
  }

  SDL_SetWindowResizable(
      sdl.window,
      false); // To prevent resizing window while the program is running

  sdl.renderer = SDL_CreateRenderer(sdl.window, NULL);
  if (!sdl.renderer) {
    fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());

    SDL_Quit();
  }

  SDL_SetRenderLogicalPresentation(
      sdl.renderer, 64, 32,
      SDL_LOGICAL_PRESENTATION_INTEGER_SCALE); // Scaling the pixels

  sdl.texture = SDL_CreateTexture(sdl.renderer, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_STREAMING, 64, 32);
  if (!sdl.texture) {
    fprintf(stderr, "Failed to load texture: %s\n", SDL_GetError());
    SDL_Quit();
  }

  // ROM is loaded

  int delay = 1;
  bool isRunning = true;

  while (isRunning) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        fprintf(stdout, "Requested close\n");
        isRunning = false;
      } else if (event.type == SDL_EVENT_KEY_DOWN ||
                 event.type == SDL_EVENT_KEY_UP) {
        handle_key_event(&event, &c8, &delay);
      }
    }

    if (delay < 0)
      delay = 0;
    SDL_Delay((uint32_t)delay);
    if (c8.delayTimer > 0)
      --c8.delayTimer;

    // Executing opcodes
    execute(&c8);

    // drawing pixels
    draw(&sdl, &c8);
  }

  return 0;
}
