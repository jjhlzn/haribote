#define _CONSOLE_WIDTH(x)  ((x)*8+16+11) 

#define CONSOLE_WIDTH_COLS 56
#define CONSOLE_HEIGHT_LINES 44
#define CONSOLE_CONTENT_WIDTH (CONSOLE_WIDTH_COLS*8)
#define CONSOLE_CONENT_HEIGHT (CONSOLE_HEIGHT_LINES*16)
#define CONSOLE_WIDTH  _CONSOLE_WIDTH(CONSOLE_WIDTH_COLS)
#define CONSOLE_HEIGHT (CONSOLE_HEIGHT_LINES*16+37)  

#define BUF_WIDTH_COLS CONSOLE_WIDTH_COLS
#define BUF_HEIGHT_LINES 300 
#define CONSOLE_BUF_CONTENT_WIDTH (BUF_WIDTH_COLS*8)
#define CONSOLE_BUF_CONTENT_HEIGHT (BUF_HEIGHT_LINES*16)
#define CONSOLE_BUF_WIDTH   _CONSOLE_WIDTH(BUF_WIDTH_COLS)
#define CONSOLE_BUF_HEIGHT (BUF_HEIGHT_LINES*16+37)

#define CONSOLE_SCROLL_BAR_WIDTH  10
#define CONSOLE_SCROLL_BAR_HEIGHT  ((CONSOLE_CONENT_HEIGHT * CONSOLE_CONENT_HEIGHT) / CONSOLE_BUF_CONTENT_HEIGHT)


void make_scroll_bar(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
