#define COLUMNS 80
#define ROWS 25

char *name = "SADOK DALIN WAS HERE";

typedef struct vga_char {
    char character; //Contains ASCII value for a character 
    char colors;    //Contains the colors that the characters should be printed with. 
} vga_char;

void myos_main(void) {
    vga_char *vga = (vga_char*)0xb8000;

    for(int i = 0; i < COLUMNS*ROWS; i++) {
        vga[i].character = ' ';
        vga[i].colors = 0x0f; //Put text and background color 
    }

    for(int i = 0; name[i] != '\0'; i++) {
        vga[600+i].character = name[i];
        vga[600+i].colors = 0x0f;
    }
}