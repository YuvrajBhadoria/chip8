#define BUFFER_SAMPLES 2048
#define FRAME_RATE 60 

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct { 
  uint8_t Memory[4096];
  uint8_t V[16];
  uint16_t ProgramCounter;
  uint16_t IndexRegister;
  bool DrawFlag;
  uint16_t Stack[16];
  uint16_t StackPointer;
  uint16_t Keyboard[16];
  uint8_t SoundTimer;
  uint8_t DelayTimer;
  uint16_t OPCode;
  uint8_t Display[32][64];
} Chip8;

typedef struct { 
  SDL_Window *Window;
  SDL_Renderer *Renderer;
  SDL_Texture *Texture;
} SDL;


static const uint8_t chip8Fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10,
    0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10,
    0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0,
    0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,
    0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80,
    0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80
  };


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

 //0x200 is the offset of the program
  size_t read = fread(c8->Memory + 0x200, 1, (size_t)size, fp);
  if (read != (size_t)size) {
    fprintf(stderr, "Failed to read ROM: read %zu of %ld\n", read, size);
    fclose(fp);
  }

  return true;
}

typedef struct {
  SDL_AudioStream *stream;
  int SampleRate;
  int Freq;
  int16_t Amplitude;
  double Phase;
  bool Playing;
} SoundState;

bool initSound(SoundState *s, int sample_rate, int freq, int16_t amplitude) {

  SDL_AudioSpec spec;
  SDL_zero(spec);
  spec.freq = sample_rate;
  spec.channels = 1;
  spec.format = SDL_AUDIO_S16;

  // Initializing struct SoundState
  s->SampleRate = sample_rate;
  s->Freq = freq;
  s->Amplitude = amplitude;
  s->Phase = 0.0;
  s->Playing = false;

  s->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                        &spec, NULL, NULL);
  if (!s->stream) {
    fprintf(stderr, "Failed to open audio stream: %s\n", SDL_GetError());
    return false;
  }

  SDL_ResumeAudioStreamDevice(s->stream);
  return true;
}

// Generating Square Waves for a Beep sound
void generateSquareWave(SoundState *s, int16_t *buffer, int n_samples) {

  double samples_per_cycle = (double)s->SampleRate / (double)s->Freq;
  double half_cycle = samples_per_cycle / 2.0;

  for (int i = 0; i < n_samples; ++i) {
    buffer[i] = (s->Phase < half_cycle) ? s->Amplitude : -s->Amplitude;

    s->Phase += 1.0;
    if (s->Phase >= samples_per_cycle) {
      s->Phase -= samples_per_cycle;
    }
  }
}

void handle_key_event(SDL_Event *ev, Chip8 *c8) {

  if (ev->type == SDL_EVENT_KEY_DOWN) {
    switch (ev->key.key) {
    case SDLK_1:
      c8->Keyboard[1] = 1;
      break;
    case SDLK_2:
      c8->Keyboard[2] = 1;
      break;
    case SDLK_3:
      c8->Keyboard[3] = 1;
      break;
    case SDLK_Q:
      c8->Keyboard[4] = 1;
      break;
    case SDLK_W:
      c8->Keyboard[5] = 1;
      break;
    case SDLK_E:
      c8->Keyboard[6] = 1;
      break;
    case SDLK_X:
      c8->Keyboard[0] = 1;
      break;
    case SDLK_A:
      c8->Keyboard[7] = 1;
      break;
    case SDLK_S:
      c8->Keyboard[8] = 1;
      break;
    case SDLK_D:
      c8->Keyboard[9] = 1;
      break;
    case SDLK_Z:
      c8->Keyboard[0xA] = 1;
      break;
    case SDLK_C:
      c8->Keyboard[0xB] = 1;
      break;
    case SDLK_4:
      c8->Keyboard[0xC] = 1;
      break;
    case SDLK_R:
      c8->Keyboard[0xD] = 1;
      break;
    case SDLK_F:
      c8->Keyboard[0xE] = 1;
      break;
    case SDLK_V:
      c8->Keyboard[0xF] = 1;
      break;
    }
  }

  else if (ev->type == SDL_EVENT_KEY_UP) {
    switch (ev->key.key) {
    case SDLK_X:
      c8->Keyboard[0] = 0;
      break;
    case SDLK_1:
      c8->Keyboard[1] = 0;
      break;
    case SDLK_2:
      c8->Keyboard[2] = 0;
      break;
    case SDLK_3:
      c8->Keyboard[3] = 0;
      break;
    case SDLK_Q:
      c8->Keyboard[4] = 0;
      break;
    case SDLK_W:
      c8->Keyboard[5] = 0;
      break;
    case SDLK_E:
      c8->Keyboard[6] = 0;
      break;
    case SDLK_A:
      c8->Keyboard[7] = 0;
      break;
    case SDLK_S:
      c8->Keyboard[8] = 0;
      break;
    case SDLK_D:
      c8->Keyboard[9] = 0;
      break;
    case SDLK_Z:
      c8->Keyboard[0xA] = 0;
      break;
    case SDLK_C:
      c8->Keyboard[0xB] = 0;
      break;
    case SDLK_4:
      c8->Keyboard[0xC] = 0;
      break;
    case SDLK_R:
      c8->Keyboard[0xD] = 0;
      break;
    case SDLK_F:
      c8->Keyboard[0xE] = 0;
      break;
    case SDLK_V:
      c8->Keyboard[0xF] = 0;
      break;
    }
  }
}

void audioUpdate(SoundState *s, bool sound_active) {


  // static so that we can resume
  static int16_t buffer[BUFFER_SAMPLES];

  if (sound_active) {
    if (!s->Playing) {
      SDL_FlushAudioStream(s->stream);
      s->Playing = true;
    }

    generateSquareWave(s, buffer, BUFFER_SAMPLES);
    int bytes = BUFFER_SAMPLES * sizeof(int16_t);

    SDL_PutAudioStreamData(s->stream, buffer, bytes);

  } else {
    if (s->Playing) {

      SDL_FlushAudioStream(s->stream);
      s->Playing = false;

      // reset waveform phase
      s->Phase = 0.0;
    }
  }
}

void execute(Chip8 *chip8) {
  uint8_t X, Y, kk, n;
  uint16_t nnn;
  uint32_t key_pressed;

  
  chip8->OPCode =
      (uint16_t)chip8->Memory[chip8->ProgramCounter] << 8 |
      chip8
          ->Memory[chip8->ProgramCounter + 1]; // converts 2 consequtive memory
                                               // element into one 16 bit number
  chip8->ProgramCounter += 2;
  X = (chip8->OPCode & 0x0F00) >> 8;
  Y = (chip8->OPCode & 0x00F0) >> 4;
  nnn = (chip8->OPCode & 0x0FFF);
  kk = (chip8->OPCode & 0x00FF);
  n = (chip8->OPCode & 0x000F);

  // printf("Opcode: %04x  PC: %04x  I: %04x\n", c8->opcode, c8->PC, c8->I);

  switch (chip8->OPCode & 0xF000) {
  case 0x0000:
    switch (chip8->OPCode & 0x00FF) {
    case 0x00E0: /* 00E0: clear screen */
      memset(chip8->Display, 0, sizeof(chip8->Display));
      chip8->DrawFlag = true;
      break;

    case 0x00EE: /* 00EE: return from subroutine */
      --chip8->StackPointer;
      chip8->ProgramCounter = chip8->Stack[chip8->StackPointer];
      break;

    default:
      printf("Unknown 0x0 opcode: %04x\n", chip8->OPCode);
      break;
    }
    break;

  case 0x1000: /* 1NNN: jump to address NNN */
    chip8->ProgramCounter = nnn;
    break;

  case 0x2000: /* 2NNN: call subroutine at NNN */
    chip8->Stack[chip8->StackPointer] = chip8->ProgramCounter;
    ++chip8->StackPointer;
    chip8->ProgramCounter = nnn;
    break;

  case 0x3000: /* 3XKK: skip next if VX == KK */
    if (chip8->V[X] == kk)
      chip8->ProgramCounter += 2;
    break;

  case 0x4000: /* 4XKK: skip next if VX != KK */
    if (chip8->V[X] != kk)
      chip8->ProgramCounter += 2;
    break;

  case 0x5000: /* 5XY0: skip next if VX == VY */
    if (chip8->V[X] == chip8->V[Y])
      chip8->ProgramCounter += 2;
    break;

  case 0x6000: /* 6XKK: VX = KK */
    chip8->V[X] = kk;
    break;

  case 0x7000: /* 7XKK: VX += KK */
    chip8->V[X] += kk;
    break;

  case 0x8000: /* 8XYN: arithmetic/bit ops */
    switch (n) {
    case 0x0:
      chip8->V[X] = chip8->V[Y];
      break;
    case 0x1:
      chip8->V[X] |= chip8->V[Y];
      break;
    case 0x2:
      chip8->V[X] &= chip8->V[Y];
      break;
    case 0x3:
      chip8->V[X] ^= chip8->V[Y];
      break;
    case 0x4: {
      chip8->V[0xF] = ((chip8->V[X] + chip8->V[Y]) > 255) ? 1 : 0;
      chip8->V[X] = chip8->V[X] + chip8->V[Y];

    } break;
    case 0x5:
      chip8->V[0xF] = (chip8->V[X] > chip8->V[Y]) ? 1 : 0;
      chip8->V[X] = chip8->V[X] - chip8->V[Y];
      break;
    case 0x6:
      chip8->V[0xF] = (chip8->V[X] & 1) == 1 ? 1 : 0;
      chip8->V[X] >>= 1;
      break;
    case 0x7:
      chip8->V[0xF] = (chip8->V[Y] > chip8->V[X]) ? 1 : 0;
      chip8->V[X] = chip8->V[Y] - chip8->V[X];
      break;
    case 0xE:
      chip8->V[0xF] = (chip8->V[X] >> 7) == 0b10000000 ? 1 : 0;
      chip8->V[X] <<= 1;
      break;
    default:
      printf("Unknown 8XYN opcode: %04x\n", chip8->OPCode);
      break;
    }
    break;

  case 0x9000: /* 9XY0: skip if VX != VY */
    if (chip8->V[X] != chip8->V[Y])
      chip8->ProgramCounter += 2;
    break;

  case 0xA000: /* ANNN: I = NNN */
    chip8->IndexRegister = nnn;
    break;

  case 0xB000: /* BNNN: jump to NNN + V0 */
    chip8->ProgramCounter = nnn + chip8->V[0x0];
    break;

  case 0xC000: /* CXKK: VX = random & KK */
    chip8->V[X] = (rand() % 256) & kk;
    break;

  case 0xD000: { /* DXYN: draw sprite at (VX, VY), height N */
    uint16_t px = chip8->V[X];
    uint16_t py = chip8->V[Y];
    uint16_t numberOfSprites = n;
    uint8_t spriteByte;

    chip8->V[0xF] = 0;

    for (uint16_t spriteNumber = 0; spriteNumber < numberOfSprites;
         spriteNumber++) {

    /*incrimenting by one from the first sprite to iterate through every
     sprite*/
      spriteByte = chip8->Memory[chip8->IndexRegister + spriteNumber];
      for (uint8_t bitNumberInSprite = 0; bitNumberInSprite < 8;
           bitNumberInSprite++) {

        // storing the value of each bit (i.e. 1 or 0)
        uint8_t spritePixel = (spriteByte >> (7 - bitNumberInSprite)) & 1;
        if (spritePixel) {
          uint16_t X = (px + bitNumberInSprite) % 64;
          uint16_t Y = (py + spriteNumber) % 32;

          if (chip8->Display[Y][X] == 1) {
            chip8->V[0xF] = 1;
          }
          chip8->Display[Y][X] ^= 1;
        }
      }
    }
    chip8->DrawFlag = true;
  } break;

  case 0xE000:
    switch (kk) {
    case 0x9E:
      if (chip8->Keyboard[chip8->V[X]] != 0)
        chip8->ProgramCounter += 2;
      break;
    case 0xA1:
      if (chip8->Keyboard[chip8->V[X]] == 0)
        chip8->ProgramCounter += 2;
      break;
    default:
      printf("Unknown E opcode: %04x\n", chip8->OPCode);
      break;
    }
    break;

  case 0xF000:
    switch (kk) {
    case 0x07:
      // FX07: VX = delay_timer
      chip8->V[X] = chip8->DelayTimer;
      break;
    case 0x0A: {
      // FX0A: wait for a key press, store in VX
      key_pressed = 0;
      for (uint32_t i = 0; i < 16; ++i) {
        if (chip8->Keyboard[i]) {
          key_pressed = 1;
          chip8->V[X] = (uint8_t)i;
        }
      }
      if (key_pressed == 0) {
        chip8->ProgramCounter -= 2;
        /* repeat this instruction until key pressed */
      }
    } break;
    case 0x15: // FX15: delay_timer = VX
      chip8->DelayTimer = chip8->V[X];

      /// askdmaksdm
      break;

      /* FX18: sound_timer = VX */
    case 0x18:
      chip8->SoundTimer = chip8->V[X];
      break;

      /* FX1E: I += VX */
    case 0x1E:
      chip8->IndexRegister = chip8->IndexRegister + chip8->V[X];
      break;

      /* FX29: I = location of sprite for digit VX */
    case 0x29:
      chip8->IndexRegister = chip8->V[X] * 5; /* each font is 5 bytes */
      break;

      /* FX33: store BCD of VX at I, I+1, I+2 */
    case 0x33: {
      int vX = chip8->V[X];
      chip8->Memory[chip8->IndexRegister] = (vX / 100) % 10;
      chip8->Memory[chip8->IndexRegister + 1] = (vX / 10) % 10;
      chip8->Memory[chip8->IndexRegister + 2] = vX % 10;
    } break;

    /* FX55: store V0..VX in memory starting at I */
    case 0x55:
      for (uint8_t i = 0; i <= X; ++i) {
        chip8->Memory[chip8->IndexRegister + i] = chip8->V[i];
      }
      break;

      /* FX65: read V0..VX from memory starting at I */
    case 0x65:
      for (uint8_t i = 0; i <= X; ++i) {
        chip8->V[i] = chip8->Memory[chip8->IndexRegister + i];
      }
      break;
    default:
      printf("Unknown F opcode: %04x\n", chip8->OPCode);
      break;
    }
    break;

  default:
    printf("Unknown opcode: %04x\n", chip8->OPCode);
    break;
  }
}

void draw(SDL *sdl, Chip8 *c8) {
  if (c8->DrawFlag) {

    SDL_SetRenderDrawColor(sdl->Renderer, 0, 0, 0, 255);
    SDL_RenderClear(sdl->Renderer);

    uint32_t drawPixel[32][64];

    // setting the drawpixel to zero (i.e. turning off all the pixels)
    memset(drawPixel, 0, sizeof(drawPixel));

    for (int py = 0; py < 32; ++py) {
      for (int px = 0; px < 64; ++px) {

        if (c8->Display[py][px] == 1) {
          // pixels getting on the location they should be
          drawPixel[py][px] = 0xFFFFFFFFu;
        }
      }
    }

    SDL_UpdateTexture(sdl->Texture, NULL, drawPixel, 64 * sizeof(uint32_t));

    SDL_FRect position;
    position.x = 0;
    position.y = 0;
    position.w = 64;
    position.h = 32;

    SDL_RenderTexture(sdl->Renderer, sdl->Texture, NULL, &position);
    SDL_RenderPresent(sdl->Renderer);
  }
  c8->DrawFlag = false;
}

void shutdownSound(SoundState *s) {
  if (s->stream) {
    SDL_DestroyAudioStream(s->stream);
    s->stream = NULL;
  }
}

void destroySDL(SDL *sdl, SoundState *sound){
  shutdownSound(sound);
  SDL_DestroyRenderer(sdl->Renderer);
  SDL_DestroyTexture(sdl->Texture);
  SDL_DestroyWindow(sdl->Window);
  SDL_Quit();
}

int main(int argc, char **argv) {

  SDL sdl = {NULL};
  Chip8 c8 = {};

  memcpy(c8.Memory, chip8_fontset, sizeof(chip8_fontset));

  if (!loadrom(&c8, argv[1])) {
    return EXIT_FAILURE;
  }

  // To Scale window according to the user
  int scale = 10; 
  c8.ProgramCounter = 0x200;

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {

    fprintf(stderr, "SDL_Intit failed: %s\n", SDL_GetError());
  }

  sdl.Window = SDL_CreateWindow("Chip8-SDL3", 64 * scale, 32 * scale, 0);
  if (!sdl.Window) {
    fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());

    SDL_Quit();
  }

  if (!SDL_SetWindowResizable(sdl.Window, false)) {
    fprintf(stderr, "SDL_SetWindowResizeable failed: %s\n", SDL_GetError());
  }

  sdl.Renderer = SDL_CreateRenderer(sdl.Window, NULL);
  if (!sdl.Renderer) {
    fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());

    SDL_Quit();
  }

  SDL_SetRenderLogicalPresentation(
      sdl.Renderer, 64, 32,
      SDL_LOGICAL_PRESENTATION_INTEGER_SCALE); 

  sdl.Texture = SDL_CreateTexture(sdl.Renderer, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_STREAMING, 64, 32);
  if (!sdl.Texture) {
    fprintf(stderr, "Failed to load texture: %s\n", SDL_GetError());
    SDL_Quit();
  }

  // Initalizing Sound
  SoundState sound = {0};
  if (!initSound(&sound, 44100, 0, 0)) {
    fprintf(stderr, "Warning: audio init failed\n");
  }

  bool isRunning = true;

  // The Program Loop
  while (isRunning) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        fprintf(stdout, "Requested close\n");
        isRunning = false;
      } else if (event.type == SDL_EVENT_KEY_DOWN ||
        event.type == SDL_EVENT_KEY_UP) {
          handle_key_event(&event, &c8);
        }
      }
      
    audioUpdate(&sound, c8.SoundTimer > 0);
    
    uint64_t firstFrame = SDL_GetTicks();

    uint32_t InstructionPerSecond = 700;

    // Uniformly execute instructions per frame
    for (int i = 0; i < InstructionPerSecond/FRAME_RATE; i++) {
      execute(&c8);
    }

    uint64_t lastFrame = SDL_GetTicks();

    //16.66666.... is the frame duration in ms
    SDL_Delay(16.66666f - (firstFrame - lastFrame));
    
    // drawing pixels
    draw(&sdl, &c8);

    //Updating Timmers
    if (c8.DelayTimer > 0) {
      --c8.DelayTimer;
    }
    if (c8.SoundTimer > 0) {
      --c8.SoundTimer;
    }
  }

  //Destroying SDL and Ending the Program
  destroySDL(&sdl , &sound);

  return 0;
}
