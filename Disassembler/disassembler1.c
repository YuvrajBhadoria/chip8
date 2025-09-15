#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint8_t V[0x10];
  uint16_t PC;
  uint16_t I;
  uint8_t DT;
  uint8_t ST;
} Chip8;

typedef struct {

  uint8_t x;
  uint8_t y;
  uint8_t kk;
  uint8_t n;
  uint8_t nnn;

} Instruction;

Instruction nextInstruction(uint16_t bin) {

  Instruction inst;

  inst.x = (0x0F00 & bin) >> 8;
  inst.y = (0x00F0 & bin) >> 4;
  inst.kk = 0x00FF & bin;
  inst.n = (0x000F & bin);
  inst.nnn = (0x0FFF & bin);

  return inst;
  
}

void decode_instruction(uint16_t bin, Chip8 c8) {

  Instruction inst = nextInstruction(bin);

  printf("%02X --- ", bin);
  if (bin >= 0x0000 && bin <= 0x0FFF) {
    if (bin == 0x00E0) {
      printf("00E0 - CLS\n");
    } else if (bin == 0x00EE) {
      printf("00EE - RET\n");
    } else if (bin >= 0x0000 && bin <= 0x0FFF) {
      printf("0nnn - SYS addr\n");
    }
  }

  if (bin >= 0x1000 && bin <= 0x1FFF) {
    printf("1nnn - JP addr\n");
  }
  if (bin >= 0x2000 && bin <= 0x2FFF) {
    printf("2nnn - CALL addr\n");
  }
  if (bin >= 0x3000 && bin <= 0x3FFF) {

    if (c8.V[inst.x] == inst.kk) {
      c8.PC += 2;
      printf("3xkk - SE Vx, byte x=%d k=%d\n", inst.x, inst.kk);
    }

  }
  if (bin >= 0x4000 && bin <= 0x4FFF) {

    if (c8.V[inst.x] != inst.kk) {

      c8.PC += 2;
      printf("4xkk - SNE Vx, byte x=%d k=%d\n", inst.x, inst.kk);
    }
  }
  if (bin >= 0x5000 && bin <= 0x5FF0) {

    if (c8.V[inst.x] == c8.V[inst.y]) {
      c8.PC += 2;

      printf("5xy0 - SE Vx, Vy x=%d y=%d\n", inst.x, inst.y);
    }
  }
  if (bin >= 0x6000 && bin <= 0x6FFF) {

    if (c8.V[inst.x] == inst.kk) {

      c8.PC += 2;

      printf("6xkk - LD Vx, byte x=%d kk=%d\n", inst.x, inst.kk);
    }
  }
  if (bin >= 0x7000 && bin <= 0x7FFF) {

    c8.V[inst.x] = c8.V[inst.x] + inst.kk;

    printf("7xkk - ADD Vx, byte x=%d k=%d\n", inst.x, inst.kk);
  }
  if (bin >= 0x8000 && bin <= 0x8FFF) {

    switch (bin & 0x000F) {
    case 0:
      printf("8xy0 - LD Vx, Vy\n");
      {
        c8.V[inst.x] = c8.V[inst.y];
      }
      break;
    case 1:
      printf("8xy1 - OR Vx, Vy\n");
      {
        c8.V[inst.x] = (c8.V[inst.x] | c8.V[inst.y]);
      }
      break;
    case 2:
      printf("8xy2 - AND Vx, Vy\n");
      {
        c8.V[inst.x] = (c8.V[inst.x] & c8.V[inst.y]);
      }
      break;
    case 3:
      printf("8xy3 - XOR Vx, Vy\n");
      {
        c8.V[inst.x] = c8.V[inst.x] ^ c8.V[inst.y];
      }
      break;

    case 4:
      printf("8xy4 - ADD Vx, Vy\n");
      {
        c8.V[inst.x] = c8.V[inst.x] + c8.V[inst.y];
      }
      break;

    case 5:
      printf("8xy5 - SUB Vx, Vy\n");
      {
        c8.V[inst.x] = c8.V[inst.x] - c8.V[inst.y];

        if (c8.V[inst.x] < c8.V[inst.y]) {
          c8.V[0xF] = 1;
        } else {
          c8.V[0xF] = 0;
        }
      }
      break;
    case 6:
      printf("8xy6 - SHR Vx {, Vy}\n");
      {
        c8.V[inst.x] = c8.V[inst.x] >> 1;

        if ((c8.V[inst.x] & 0x1) == 0x1) {
          c8.V[0xF] = 1;
        } else {
          c8.V[0xF] = 0;
        }
      }
      break;
    case 7:
      printf("8xy7 - SUBN Vx, Vy\n");
      {
        c8.V[inst.x] = c8.V[inst.y] - c8.V[inst.x];
        if (c8.V[inst.x] > c8.V[inst.y]) {
          c8.V[0xF] = 1;
        } else {
          c8.V[0xF] = 0;
        }
        c8.V[inst.x] / 2;
      }
      break;
    case 0xE:
      printf("8xyE - SHL Vx {, Vy}\n");
      {
        c8.V[inst.x] = c8.V[inst.x] << 1;

        if ((c8.V[inst.x] & 0b10000000) == 0b10000000) {
          c8.V[0xF] = 1;
        } else {
          c8.V[0xF] = 0;
        }
        c8.V[inst.x] * 2;
      }
      break;
    }
  }
  if (bin >= 0x9000 && bin <= 0x9FF0) {
    if (c8.V[inst.x] != c8.V[inst.y]) {
      c8.PC += 2;
    }

    printf("9xy0 - SNE Vx, Vy\n");
  }

  switch (bin & 0xF000) {
  case 0xA000:
    printf("Annn - LD I, addr\n");
    {
      c8.I = inst.nnn;
    }
    break;
  case 0xB000:
    printf("Bnnn - JP V0, addr\n");
    {
      c8.PC = inst.nnn + c8.V[0];
    }
    break;
  case 0xC000:
    printf("Cxkk - RND Vx, byte\n");
    {

      int num = rand() % 256;
      c8.V[inst.x] = num & inst.kk;
    }
    break;
  case 0xD000:
    printf("Dxyn - DRW Vx, Vy, nibble\n");
    {

      // GRAPHICS
    }
    break;
  case 0xE000:
    if ((bin & 0x00FF) == 0x009E) {

      printf("Ex9E - SKP Vx\n");
    } else {
      printf("ExA1 - SKNP Vx\n");
    }
    break;
  case 0xF000:
    if ((bin & 0x00FF) == 0x0007) {
      printf("Fx07 - LD Vx, DT\n");
      c8.V[inst.x] = c8.DT;
    }
    if ((bin & 0x00FF) == 0x000A)
      printf("Fx0A - LD Vx, K\n");
    if ((bin & 0x00FF) == 0x0015) {

      c8.DT = c8.V[inst.x];
      printf("Fx15 - LD DT, Vx\n");
    }

    if ((bin & 0x00FF) == 0x0018) {

      c8.ST = c8.V[inst.x];
      printf("Fx18 - LD ST, Vx\n");
    }
    if ((bin & 0x00FF) == 0x001E) {
      c8.I += c8.V[inst.x];
      printf("Fx1E - ADD I, Vx\n");
    }
    if ((bin & 0x00FF) == 0x0029)
      printf("Fx29 - LD F, Vx\n");
    if ((bin & 0x00FF) == 0x0033)
      printf("Fx33 - LD B, Vx\n");
    if ((bin & 0x00FF) == 0x0055) {
      c8.V[0] = c8.V[inst.x];
      // something to do with I
      c8.I += inst.x + 1;
      printf("Fx55 - LD [I], Vx\n");
    }
    if ((bin & 0x00FF) == 0x0065)
      printf("Fx65 - LD Vx, [I]\n");
    break;
  }
}

void cleanup_sdl(void) {
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  SDL_Quit();
}

void init_sdl(void) {
  bool success = SDL_Init(SDL_INIT_VIDEO);
  if (success != true) {
    fprintf(stderr,
            "Following error occured when trying to initialize SDL Video "
            "subsystem: %s\n",
            SDL_GetError());
    cleanup_sdl();
    exit(EXIT_FAILURE);
  }

  success = SDL_SetAppMetadata("Chip8", "1.0", "com.yuvrajbhadoria.chip8");
  if (success != true) {
    fprintf(stderr, "Following error occured when trying to set metadata: %s\n",
            SDL_GetError());
  }
}

int main() {
  Instruction inst;
  Chip8 c8;
  FILE *fp = fopen("./CUBE8.ch8", "rb");
  if (!fp) {
    printf("Error opening file\n");
    return 1;
  }

  fseek(fp, 0, SEEK_END);
  uint8_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  uint8_t *arr = (uint8_t *)malloc(size);
  if (!arr) {
    printf("Error allocating memory\n");
    fclose(fp);
    return 1;
  }

  fread(arr, sizeof(arr[0]), size, fp);

  // Read instructions 2 bytes at a time
  for (int i = 0; i < size; i += 2) {
    uint16_t bin =
        (arr[i] << 8) | arr[i + 1]; // combine bytes into a 16-bit instruction
    decode_instruction(bin, c8);
  }

  free(arr);
  fclose(fp);

  init_sdl();
  // To Scale window
  int scale;
  scanf("%d",&scale);
  SDL_Window *window = SDL_CreateWindow("Chip8", 64*scale,32*scale,0);

  if (window == NULL) {
    fprintf(stderr,
            "Following error occured when trying to create a window: %s\n",
            SDL_GetError());
    cleanup_sdl();
    exit(EXIT_FAILURE);
  }

  SDL_SetWindowResizable(window,false);

  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
    cleanup_sdl();
    exit(EXIT_FAILURE);
  }



  SDL_SetRenderLogicalPresentation(renderer, 64, 32,SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
   
  bool is_running = true;

  while (is_running) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        fprintf(stdout, "Requested close\n");
        is_running = false;
      }
    }
    SDL_FRect rect = {0, 0, 1, 1};

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderClear(renderer);


    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &rect);

    
    SDL_RenderPresent(renderer);

  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  cleanup_sdl();
// woohoooo checking commits :) sorry frothy for being annoying and Dumb :) <3 !!
  return 0;
}
