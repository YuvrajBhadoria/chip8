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

      #define SCREEN_WIDTH 64
      #define SCREEN_HEIGHT 32
      uint8_t memory[4096];
      uint8_t keyboard[16];

      uint8_t display[SCREEN_HEIGHT][SCREEN_WIDTH];
      bool keys[16];
      bool drawflag = false;

      bool waiting_for_key = false;
      uint8_t waiting_vx_index = 0;
      uint32_t key_pressed;

      typedef struct {
        uint8_t V[16];
        uint16_t PC;
        uint16_t I;
        uint16_t stack[16];
        uint8_t sp;
        uint8_t delayTimer;
        uint8_t soundTimer;
      } Chip8;
      
      typedef struct {

        uint8_t x;
        uint8_t y;
        uint8_t kk;
        uint8_t n;
        uint16_t nnn;

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
      static const uint8_t chip8_fontset[80] = {
    0xF0,0x90,0x90,0x90,0xF0, 
    0x20,0x60,0x20,0x20,0x70, 
    0xF0,0x10,0xF0,0x80,0xF0, 
    0xF0,0x10,0xF0,0x10,0xF0, 
    0x90,0x90,0xF0,0x10,0x10, 
    0xF0,0x80,0xF0,0x10,0xF0, 
    0xF0,0x80,0xF0,0x90,0xF0, 
    0xF0,0x10,0x20,0x40,0x40, 
    0xF0,0x90,0xF0,0x90,0xF0, 
    0xF0,0x90,0xF0,0x10,0xF0, 
    0xF0,0x90,0xF0,0x90,0x90, 
    0xE0,0x90,0xE0,0x90,0xE0, 
    0xF0,0x80,0x80,0x80,0xF0, 
    0xE0,0x90,0x90,0x90,0xE0, 
    0xF0,0x80,0xF0,0x80,0xF0, 
    0xF0,0x80,0xF0,0x80,0x80  
  };
    void updateScreen(SDL_Renderer *renderer) {
        if (!drawflag)
          return;

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

              
          for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                if (display[y][x]) {
                    SDL_FRect rect = {
                        .x = (float)x,
                        .y = (float)y,
                        .w = 1,
                        .h = 1
                    };
                    SDL_RenderFillRect(renderer, &rect);

                }
            }
        }
                  


        SDL_RenderPresent(renderer);
        drawflag = false;
      }    

      void decode_instruction(uint16_t bin, Chip8 *c8, SDL_Renderer *renderer) {

        Instruction inst = nextInstruction(bin);

        printf("%02X --- ", bin);
        if (bin >= 0x0000 && bin <= 0x0FFF) {
          if (bin == 0x00E0) {

            memset(display, 0, sizeof(display));
            drawflag = true;

            printf("00E0 - CLS\n");
            c8->PC+=2;
          } else if (bin == 0x00EE) {
            printf("00EE - RET\n");
            --c8->sp;
            c8->PC = c8->stack[c8->sp];
        
          } else if (bin >= 0x0000 && bin <= 0x0FFF) {
            printf("0nnn - SYS addr\n");
            c8->PC+=2;
          }
        }

        if (bin >= 0x1000 && bin <= 0x1FFF) {
          printf("1nnn - JP addr\n");
          c8->PC = inst.nnn;
          
        }
        if (bin >= 0x2000 && bin <= 0x2FFF) {
          printf("2nnn - CALL addr\n");
          uint16_t address = bin & 0x0FFF;

          c8->stack[c8->sp] = c8->PC;
            ++c8->sp;
              c8->PC = address;
        }
        if (bin >= 0x3000 && bin <= 0x3FFF) {

          if (c8->V[inst.x] == inst.kk) {
            c8->PC += 4; //skip 
          }
          else{
              c8->PC+=2;
          }
          

            printf("3xkk - SE Vx, byte x=%d k=%d\n", inst.x, inst.kk);
          
        }
        if (bin >= 0x4000 && bin <= 0x4FFF) {

          if (c8->V[inst.x] != inst.kk) {
            c8->PC += 4;
          } 
          else {
              c8->PC+=2;
          }     
            printf("4xkk - SNE Vx, byte x=%d k=%d\n", inst.x, inst.kk);
          
        }
        if (bin >= 0x5000 && bin <= 0x5FF0) {

          if (c8->V[inst.x] == c8->V[inst.y]) {
            c8->PC += 4;
          }
          else {
              c8->PC+=2;
          }  

            printf("5xy0 - SE Vx, Vy x=%d y=%d\n", inst.x, inst.y);
          }
        
        if (bin >= 0x6000 && bin <= 0x6FFF) {

            c8->V[inst.x] = inst.kk;

              c8->PC+=2;

            printf("6xkk - LD Vx, byte x=%d kk=%d\n", inst.x, inst.kk);
          
        }
        if (bin >= 0x7000 && bin <= 0x7FFF) {

          c8->V[inst.x] = c8->V[inst.x] + inst.kk;

            c8->PC+=2;
          printf("7xkk - ADD Vx, byte x=%d k=%d\n", inst.x, inst.kk);
      
        }
        if (bin >= 0x8000 && bin <= 0x8FFF) {

          switch (bin & 0x000F) {
          case 0:
            printf("8xy0 - LD Vx, Vy\n");
            {
              c8->V[inst.x] = c8->V[inst.y];
                c8->PC+=2;
            }
            break;
          case 1:
            printf("8xy1 - OR Vx, Vy\n");
            {
              c8->V[inst.x] = (c8->V[inst.x] | c8->V[inst.y]);
            c8->PC+=2;
            }
            break;
          case 2:
            printf("8xy2 - AND Vx, Vy\n");
            {
              c8->V[inst.x] = (c8->V[inst.x] & c8->V[inst.y]);
                  c8->PC+=2;
            }
            break;
          case 3:
            printf("8xy3 - XOR Vx, Vy\n");
            {
              c8->V[inst.x] = c8->V[inst.x] ^ c8->V[inst.y];
                c8->PC+=2;
            }
            break;

          case 4:
            printf("8xy4 - ADD Vx, Vy\n");
            {
              c8->V[inst.x] = c8->V[inst.x] + c8->V[inst.y];
                c8->PC+=2;
            }
            break;

          case 5:
            printf("8xy5 - SUB Vx, Vy\n");
            {
              

              if (c8->V[inst.x] > c8->V[inst.y]) {
                c8->V[0xF] = 1;
              } else {
                c8->V[0xF] = 0;
              }
              
              c8->V[inst.x] = c8->V[inst.x] - c8->V[inst.y];
                c8->PC+=2;
            
            }
            break;
          case 6:
            printf("8xy6 - SHR Vx {, Vy}\n");
            {
              
            

              if ((c8->V[inst.x] & 0x1) == 0x1) {
                c8->V[0xF] = 1;
              } else {
                c8->V[0xF] = 0;
              }
              c8->V[inst.x] = c8->V[inst.x] >> 1;
                c8->PC+=2;
        
            }
            break;
          case 7:
            printf("8xy7 - SUBN Vx, Vy\n");
            {
              
              if (c8->V[inst.x] < c8->V[inst.y]) {
                c8->V[0xF] = 1;
              } else {
                c8->V[0xF] = 0;
              }
              c8->V[inst.x] = c8->V[inst.y] - c8->V[inst.x];
                c8->PC+=2;
              
    
            }
            break;
          case 0xE:
            printf("8xyE - SHL Vx {, Vy}\n");
            {
              

              if ((c8->V[inst.x] & 0b10000000) == 0b10000000) {
                c8->V[0xF] = 1;
              } else {
                c8->V[0xF] = 0;
              }
              c8->V[inst.x] = c8->V[inst.x] << 1;
                c8->PC+=2;
      
            }
            break;
          }
        }
        if (bin >= 0x9000 && bin <= 0x9FF0) {
          if (c8->V[inst.x] != c8->V[inst.y]) {
            c8->PC += 4;
          }
          else {
              c8->PC+=2;
          }  

          printf("9xy0 - SNE Vx, Vy\n");
        }

        switch (bin & 0xF000) {
        case 0xA000:
          printf("Annn - LD I, addr\n");
          {
            c8->I = inst.nnn;
              c8->PC+=2;
          }
          break;
        case 0xB000:
          printf("Bnnn - JP V0, addr\n");
          {
            c8->PC = inst.nnn + c8->V[0];
            
          }
          break;
        case 0xC000:
          printf("Cxkk - RND Vx, byte\n");
          {

            int num = rand() % 256;
            c8->V[inst.x] = num & inst.kk;
              c8->PC+=2;
          
          }
          break;
        case 0xD000:
          printf("Dxyn - DRW Vx, Vy, nibble\n");
          {
            uint8_t x = c8->V[inst.x];
            uint8_t y = c8->V[inst.y];
            uint8_t numberOfSprites = inst.n;
            c8->V[0xF] = 0;

            for (int spritePosition = 0; spritePosition < numberOfSprites;
                spritePosition++) {
              uint8_t spriteByte = memory[c8->I + spritePosition];

              for (int spritebit = 0; spritebit < 8; spritebit++) {
             
                int pixel_x = (x + spritebit) % SCREEN_WIDTH;
                int pixel_y = (y + spritePosition) % SCREEN_HEIGHT;

                uint8_t spritePixel = (spriteByte >> (7 - spritebit)) & 1;



                if (spritePixel) {
                  if (display[pixel_y][pixel_x] == 1) {
                    c8->V[0xF] = 1;
                  }
                  display[pixel_y][pixel_x] ^= 1;
                }
              }
            }

            drawflag = true;
            c8->PC+=2;
            // updateScreen(renderer);
          }
          break;
        case 0xE000:
      if ((bin & 0x00FF) == 0x009E) { // SKP Vx
          printf("Ex9E - SKP Vx\n");
          if (keyboard[c8->V[inst.x]] != 0) {
              c8->PC += 4;
          } else {
              c8->PC += 2;
          }
      } else if ((bin & 0x00FF) == 0x00A1) { // SKNP Vx
          printf("ExA1 - SKNP Vx\n");
          if (keyboard[c8->V[inst.x]]) {
              c8->PC += 4;
          } else {
              c8->PC += 2;
          }
      }
      break;

        case 0xF000:
          if ((bin & 0x00FF) == 0x0007) {
            printf("Fx07 - LD Vx, DT\n");
            c8->V[inst.x] = c8->delayTimer;
              c8->PC+=2;
        
          }
          if ((bin & 0x00FF) == 0x000A)
            printf("Fx0A - LD Vx, K\n");

              key_pressed = 0;
              for (int i = 0; i < 16 ; ++i) {
                  if(keyboard[i]){
                      key_pressed = 1;
                      c8->V[inst.x] = (uint8_t)i;
                  }
                  if(key_pressed != 0){
                      c8->PC += 2;
                  }

              }  


      
          if ((bin & 0x00FF) == 0x0015) {

            c8->delayTimer = c8->V[inst.x];
              c8->PC+=2;
            printf("Fx15 - LD DT, Vx\n");
        
          }

          if ((bin & 0x00FF) == 0x0018) {

            c8->soundTimer = c8->V[inst.x];
              c8->PC+=2;
            printf("Fx18 - LD ST, Vx\n");
          
          }
          if ((bin & 0x00FF) == 0x001E) {
            c8->I += c8->V[inst.x];
              c8->PC+=2;
            printf("Fx1E - ADD I, Vx\n");
        
          }
          if ((bin & 0x00FF) == 0x0029)
          printf("Fx29 - LD F, Vx\n");
            c8->I = c8->V[inst.x] * 5;
              c8->PC+=2;
        
          if ((bin & 0x00FF) == 0x0033)
            printf("Fx33 - LD B, Vx\n");
        
              memory[c8->I] = c8->V[inst.x] / 100;        
              memory[c8->I + 1] = (c8->V[inst.x] / 10) % 10;  
              memory[c8->I + 2] = c8->V[inst.x] % 10;
                c8->PC+=2;
        
          if ((bin & 0x00FF) == 0x0055) {

            for (int i = 0; i <= inst.x; i++) {
              memory[c8->I + i] = c8->V[i];
            }
            
            c8->I += inst.x + 1;
              c8->PC+=2;

            printf("Fx55 - LD [I], Vx\n");

          }
          if ((bin & 0x00FF) == 0x0065) {
            printf("Fx65 - LD Vx, [I]\n");
            
            for (int i = 0; i <= inst.x; i++) {
              c8->V[i] = memory[c8->I + i];
            }
            c8->I += inst.x + 1;
              c8->PC+=2;
        
          }

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
  

  void handle_key_event(SDL_Event *ev, Chip8 *c8,int delay) {
     
     
      if(ev->type == SDL_EVENT_KEY_DOWN)
      {

          switch (ev->key.key) {

                        case SDLK_F3: delay += 1; break;
                        case SDLK_F2: delay -= 1; break;
                        case SDLK_0: keyboard[0] = 1; break;
                        case SDLK_1: keyboard[1] = 1; break;
                        case SDLK_2: keyboard[2] = 1; break;
                        case SDLK_3: keyboard[3] = 1; break;
                        case SDLK_Q: keyboard[4] = 1; break;
                        case SDLK_W: keyboard[5] = 1; break;
                        case SDLK_E: keyboard[6] = 1; break;
                        case SDLK_A: keyboard[7] = 1; break;
                        case SDLK_S: keyboard[8] = 1; break;
                        case SDLK_D: keyboard[9] = 1; break;
                        case SDLK_Z: keyboard[0xA] = 1; break;
                        case SDLK_C: keyboard[0xB] = 1; break;
                        case SDLK_4: keyboard[0xC] = 1; break;
                        case SDLK_R: keyboard[0xD] = 1; break;
                        case SDLK_F: keyboard[0xE] = 1; break;
                        case SDLK_V: keyboard[0xF] = 1; break;
                    }
                    
          }      
      else if (ev->type == SDL_EVENT_KEY_UP) {
              switch (ev->key.key) {
                  case SDLK_X: keyboard[0] = 0; break;
                        case SDLK_1: keyboard[1] = 0; break;
                        case SDLK_2: keyboard[2] = 0; break;
                        case SDLK_3: keyboard[3] = 0; break;
                        case SDLK_Q: keyboard[4] = 0; break;
                        case SDLK_W: keyboard[5] = 0; break;
                        case SDLK_E: keyboard[6] = 0; break;
                        case SDLK_A: keyboard[7] = 0; break;
                        case SDLK_S: keyboard[8] = 0; break;
                        case SDLK_D: keyboard[9] = 0; break;
                        case SDLK_Z: keyboard[0xA] =0; break;
                        case SDLK_C: keyboard[0xB] = 0; break;
                        case SDLK_4: keyboard[0xC] = 0; break;
                        case SDLK_R: keyboard[0xD] = 0; break;
                        case SDLK_F: keyboard[0xE] = 0; break;
                        case SDLK_V: keyboard[0xF] = 0; break;
              }
         }
      }
  
  uint32_t last_timer_tick = 0;

  void tick_timers(Chip8 *c8) {
      uint32_t now = SDL_GetTicks();
      if (now - last_timer_tick >= 16) { 
          if (c8->delayTimer > 0) c8->delayTimer--;
          if (c8->soundTimer > 0) {
              c8->soundTimer--;
          }
          last_timer_tick = now;
      }
  }


      int main() {

        Instruction inst;
        Chip8 c8;
        c8.delayTimer = 0;
        c8.soundTimer = 0;
        c8.I = 0;
         memset(keyboard, 0, sizeof(keyboard));
        memset(c8.V,0,sizeof(c8.V));
        memset(c8.stack, 0, sizeof(c8.stack));
        memcpy(memory + 0x50, chip8_fontset, sizeof(chip8_fontset));
        c8.sp = 0;
        waiting_for_key = false;
        FILE *fp = fopen("./CUBE8.ch8", "rb");
        if (!fp) {
          printf("Error opening file\n");
          return 1;
        }

        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        fread(memory + 0x200, sizeof(memory[0]), size, fp);
      
        fclose(fp);

        init_sdl();
        // To Scale window
        int scale ;
        printf("Enter the scale size (scale>1): ");
        scanf("%d", &scale);
        
        SDL_Window *window = SDL_CreateWindow("Fila is smart according to Frothy", 64*scale , 32*scale , 0);

        if (window == NULL) {
          fprintf(stderr,
                  "Following error occured when trying to create a window: %s\n",
                  SDL_GetError());
          cleanup_sdl();
          exit(EXIT_FAILURE);
        }
        SDL_SetWindowResizable(window, false);

        SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
        if (!renderer) {
          fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
          cleanup_sdl();
          exit(EXIT_FAILURE);
        }

        SDL_SetRenderLogicalPresentation(renderer, 64, 32,SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

        bool is_running = true;

        int delay = 1;

        int instructionPerFrame = 20;
        c8.PC = 0x200;

        uint32_t lastCycle = SDL_GetTicks();


        while (is_running) {
          SDL_Event event;

          while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
              fprintf(stdout, "Requested close\n");
              is_running = false;
            }else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                  handle_key_event(&event, &c8,delay); 
              }
          
          }

          if(delay < 0) delay = 0;
          SDL_Delay((uint32_t) delay);

          if(c8.delayTimer > 0 ) --c8.delayTimer;
          
          uint16_t bin = (memory[c8.PC] << 8) | memory[c8.PC + 1];
          decode_instruction(bin, &c8,renderer);
              
              updateScreen(renderer);
         
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        cleanup_sdl();
        
        

        return 0;
      }
