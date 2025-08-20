#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

void decode_instruction(uint16_t bin) {
    int hex1 = 0x0FF0;
    int hex2 = 0x00F0;
    int hex3 = 0x0FFF;
    int hex4 = 0x0F00;
    int hex5 = 0x000F;
      printf("%02X --- ",bin);
    if (bin >= 0x0000 && bin <= 0x0FFF) {
        if (bin == 0x00E0) {
            printf("00E0 - CLS\n");
        }
        else if (bin == 0x00EE) {
            printf("00EE - RET\n");
        }
        else if (bin >= 0x0000 && bin <=0x0FFF) {
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
        int k = (bin & hex2) >> 4;
        int x = (bin & hex1) >> 8;
        int k1 = (bin & hex5);
        if (k1 == k) {
            printf("3xkk - SE Vx, byte x=%d k=%d\n", x, k);
        }
    }
    if (bin >= 0x4000 && bin <= 0x4FFF) {
        int k = (bin & hex2) >> 4;
        int x = (bin & hex1) >> 8;
        int k1 = (bin & hex5);
        if (k1 == k) {
            printf("4xkk - SNE Vx, byte x=%d k=%d\n", x, k);
        }
    }
    if (bin >= 0x5000 && bin <= 0x5FF0) {
        int x = (bin & hex1) >> 8;
        int y = (bin & hex2) >> 4;
        printf("5xy0 - SE Vx, Vy x=%d y=%d\n", x, y);
    }
    if (bin >= 0x6000 && bin <= 0x6FFF) {
        int k = (bin & hex2) >> 4;
        int x = (bin & hex1) >> 8;
        int k1 = (bin & hex5);
        
            printf("6xkk - LD Vx, byte x=%d k=%d k1=%d\n", x, k,k1);
        
    }
    if (bin >= 0x7000 && bin <= 0x7FFF) {
        int k = (bin & hex2) >> 4;
        int x = (bin & hex1) >> 8;
        int k1 = (bin & hex5);
        
            printf("7xkk - ADD Vx, byte x=%d k=%d\n", x, k);
        
    }
    if (bin >= 0x8000 && bin <= 0x8FFF) {
        switch (bin & 0x000F) {
            case 0: printf("8xy0 - LD Vx, Vy\n"); break;
            case 1: printf("8xy1 - OR Vx, Vy\n"); break;
            case 2: printf("8xy2 - AND Vx, Vy\n"); break;
            case 3: printf("8xy3 - XOR Vx, Vy\n"); break;
            case 4: printf("8xy4 - ADD Vx, Vy\n"); break;
            case 5: printf("8xy5 - SUB Vx, Vy\n"); break;
            case 6: printf("8xy6 - SHR Vx {, Vy}\n"); break;
            case 7: printf("8xy7 - SUBN Vx, Vy\n"); break;
            case 0xE: printf("8xyE - SHL Vx {, Vy}\n"); break;
        }
    }
    if (bin >= 0x9000 && bin <= 0x9FF0) {
        printf("9xy0 - SNE Vx, Vy\n");
    }

    switch (bin & 0xF000) {
        case 0xA000: printf("Annn - LD I, addr\n"); break;
        case 0xB000: printf("Bnnn - JP V0, addr\n"); break;
        case 0xC000: printf("Cxkk - RND Vx, byte\n"); break;
        case 0xD000: printf("Dxyn - DRW Vx, Vy, nibble\n"); break;
        case 0xE000:
            if ((bin & 0x00FF) == 0x009E) printf("Ex9E - SKP Vx\n");
            else printf("ExA1 - SKNP Vx\n");
            break;
        case 0xF000:
            if ((bin & 0x00FF) == 0x0007) printf("Fx07 - LD Vx, DT\n");
            if ((bin & 0x00FF) == 0x000A) printf("Fx0A - LD Vx, K\n");
            if ((bin & 0x00FF) == 0x0015) printf("Fx15 - LD DT, Vx\n");
            if ((bin & 0x00FF) == 0x0018) printf("Fx18 - LD ST, Vx\n");
            if ((bin & 0x00FF) == 0x001E) printf("Fx1E - ADD I, Vx\n");
            if ((bin & 0x00FF) == 0x0029) printf("Fx29 - LD F, Vx\n");
            if ((bin & 0x00FF) == 0x0033) printf("Fx33 - LD B, Vx\n");
            if ((bin & 0x00FF) == 0x0055) printf("Fx55 - LD [I], Vx\n");
            if ((bin & 0x00FF) == 0x0065) printf("Fx65 - LD Vx, [I]\n");
            break;
    }
}

int main() {
    FILE* fp = fopen("C:\\Users\\byyuv\\Downloads\\CUBE8.ch8", "rb");
    if (!fp) {
        printf("Error opening file\n");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint8_t* arr = (uint8_t*) malloc(size);
    if (!arr) {
        printf("Error allocating memory\n");
        fclose(fp);
        return 1;
    }

    fread(arr, sizeof(arr[0]), size, fp);

    // Read instructions 2 bytes at a time
    for (int i = 0; i < size; i += 2) {
        uint16_t bin = (arr[i] << 8) | arr[i + 1]; // combine bytes into a 16-bit instruction
        decode_instruction(bin);
    }

    free(arr);
    fclose(fp);
    return 0;
}
