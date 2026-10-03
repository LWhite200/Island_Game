// font.c
#include "font.h"


// ============================================================
// 5x7 Bitmap Font
//
// Each character consists of 7 rows.
// Each row uses 5 bits.
//
// Example:
//
//     01110
//     10001
//     10001
//     11111
//     10001
//     10001
//     10001
//
// 1 = filled pixel
// 0 = empty pixel
//
// Characters are stored as:
//
//     { row0, row1, row2, row3, row4, row5, row6 }
//
// ============================================================


// ============================================================
// Numbers 0-9
// ============================================================

static const unsigned char FONT_0[7] = {
    0b01110,
    0b10001,
    0b10011,
    0b10101,
    0b11001,
    0b10001,
    0b01110
};

static const unsigned char FONT_1[7] = {
    0b00100,
    0b01100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b01110
};

static const unsigned char FONT_2[7] = {
    0b01110,
    0b10001,
    0b00001,
    0b00010,
    0b00100,
    0b01000,
    0b11111
};

static const unsigned char FONT_3[7] = {
    0b11110,
    0b00001,
    0b00001,
    0b01110,
    0b00001,
    0b00001,
    0b11110
};

static const unsigned char FONT_4[7] = {
    0b00010,
    0b00110,
    0b01010,
    0b10010,
    0b11111,
    0b00010,
    0b00010
};

static const unsigned char FONT_5[7] = {
    0b11111,
    0b10000,
    0b10000,
    0b11110,
    0b00001,
    0b00001,
    0b11110
};

static const unsigned char FONT_6[7] = {
    0b01110,
    0b10000,
    0b10000,
    0b11110,
    0b10001,
    0b10001,
    0b01110
};

static const unsigned char FONT_7[7] = {
    0b11111,
    0b00001,
    0b00010,
    0b00100,
    0b01000,
    0b01000,
    0b01000
};

static const unsigned char FONT_8[7] = {
    0b01110,
    0b10001,
    0b10001,
    0b01110,
    0b10001,
    0b10001,
    0b01110
};

static const unsigned char FONT_9[7] = {
    0b01110,
    0b10001,
    0b10001,
    0b01111,
    0b00001,
    0b00001,
    0b01110
};


// ============================================================
// Lowercase a-z
// ============================================================

static const unsigned char FONT_a[7] = {
    0b00000,
    0b00000,
    0b01110,
    0b00001,
    0b01111,
    0b10001,
    0b01111
};

static const unsigned char FONT_b[7] = {
    0b10000,
    0b10000,
    0b10110,
    0b11001,
    0b10001,
    0b10001,
    0b11110
};

static const unsigned char FONT_c[7] = {
    0b00000,
    0b00000,
    0b01111,
    0b10000,
    0b10000,
    0b10000,
    0b01111
};

static const unsigned char FONT_d[7] = {
    0b00001,
    0b00001,
    0b01101,
    0b10011,
    0b10001,
    0b10001,
    0b01111
};

static const unsigned char FONT_e[7] = {
    0b00000,
    0b00000,
    0b01110,
    0b10001,
    0b11111,
    0b10000,
    0b01110
};

static const unsigned char FONT_f[7] = {
    0b00110,
    0b01001,
    0b01000,
    0b11100,
    0b01000,
    0b01000,
    0b01000
};

static const unsigned char FONT_g[7] = {
    0b00000,
    0b00000,
    0b01111,
    0b10001,
    0b01111,
    0b00001,
    0b11110
};

static const unsigned char FONT_h[7] = {
    0b10000,
    0b10000,
    0b10110,
    0b11001,
    0b10001,
    0b10001,
    0b10001
};

static const unsigned char FONT_i[7] = {
    0b00100,
    0b00000,
    0b01100,
    0b00100,
    0b00100,
    0b00100,
    0b01110
};

static const unsigned char FONT_j[7] = {
    0b00010,
    0b00000,
    0b00110,
    0b00010,
    0b00010,
    0b10010,
    0b01100
};

static const unsigned char FONT_k[7] = {
    0b10000,
    0b10000,
    0b10010,
    0b10100,
    0b11000,
    0b10100,
    0b10010
};

static const unsigned char FONT_l[7] = {
    0b01100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b01110
};

static const unsigned char FONT_m[7] = {
    0b00000,
    0b00000,
    0b11010,
    0b10101,
    0b10101,
    0b10101,
    0b10101
};

static const unsigned char FONT_n[7] = {
    0b00000,
    0b00000,
    0b10110,
    0b11001,
    0b10001,
    0b10001,
    0b10001
};

static const unsigned char FONT_o[7] = {
    0b00000,
    0b00000,
    0b01110,
    0b10001,
    0b10001,
    0b10001,
    0b01110
};

static const unsigned char FONT_p[7] = {
    0b00000,
    0b00000,
    0b11110,
    0b10001,
    0b11110,
    0b10000,
    0b10000
};

static const unsigned char FONT_q[7] = {
    0b00000,
    0b00000,
    0b01111,
    0b10001,
    0b01111,
    0b00001,
    0b00001
};

static const unsigned char FONT_r[7] = {
    0b00000,
    0b00000,
    0b10110,
    0b11001,
    0b10000,
    0b10000,
    0b10000
};

static const unsigned char FONT_s[7] = {
    0b00000,
    0b00000,
    0b01111,
    0b10000,
    0b01110,
    0b00001,
    0b11110
};

static const unsigned char FONT_t[7] = {
    0b01000,
    0b01000,
    0b11110,
    0b01000,
    0b01000,
    0b01001,
    0b00110
};

static const unsigned char FONT_u[7] = {
    0b00000,
    0b00000,
    0b10001,
    0b10001,
    0b10001,
    0b10011,
    0b01101
};

static const unsigned char FONT_v[7] = {
    0b00000,
    0b00000,
    0b10001,
    0b10001,
    0b10001,
    0b01010,
    0b00100
};

static const unsigned char FONT_w[7] = {
    0b00000,
    0b00000,
    0b10001,
    0b10001,
    0b10101,
    0b10101,
    0b01010
};

static const unsigned char FONT_x[7] = {
    0b00000,
    0b00000,
    0b10001,
    0b01010,
    0b00100,
    0b01010,
    0b10001
};

static const unsigned char FONT_y[7] = {
    0b00000,
    0b00000,
    0b10001,
    0b10001,
    0b01111,
    0b00001,
    0b11110
};

static const unsigned char FONT_z[7] = {
    0b00000,
    0b00000,
    0b11111,
    0b00010,
    0b00100,
    0b01000,
    0b11111
};


// ============================================================
// Uppercase A-Z
//
// These are included too so debug text can use normal
// uppercase letters without needing to convert strings.
// ============================================================

static const unsigned char FONT_A[7] = {
    0b01110,
    0b10001,
    0b10001,
    0b11111,
    0b10001,
    0b10001,
    0b10001
};

static const unsigned char FONT_B[7] = {
    0b11110,
    0b10001,
    0b10001,
    0b11110,
    0b10001,
    0b10001,
    0b11110
};

static const unsigned char FONT_C[7] = {
    0b01111,
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b01111
};

static const unsigned char FONT_D[7] = {
    0b11110,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11110
};

static const unsigned char FONT_E[7] = {
    0b11111,
    0b10000,
    0b10000,
    0b11110,
    0b10000,
    0b10000,
    0b11111
};

static const unsigned char FONT_F[7] = {
    0b11111,
    0b10000,
    0b10000,
    0b11110,
    0b10000,
    0b10000,
    0b10000
};

static const unsigned char FONT_G[7] = {
    0b01111,
    0b10000,
    0b10000,
    0b10111,
    0b10001,
    0b10001,
    0b01111
};

static const unsigned char FONT_H[7] = {
    0b10001,
    0b10001,
    0b10001,
    0b11111,
    0b10001,
    0b10001,
    0b10001
};

static const unsigned char FONT_I[7] = {
    0b01110,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b01110
};

static const unsigned char FONT_J[7] = {
    0b00111,
    0b00010,
    0b00010,
    0b00010,
    0b10010,
    0b10010,
    0b01100
};

static const unsigned char FONT_K[7] = {
    0b10001,
    0b10010,
    0b10100,
    0b11000,
    0b10100,
    0b10010,
    0b10001
};

static const unsigned char FONT_L[7] = {
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b11111
};

static const unsigned char FONT_M[7] = {
    0b10001,
    0b11011,
    0b10101,
    0b10101,
    0b10001,
    0b10001,
    0b10001
};

static const unsigned char FONT_N[7] = {
    0b10001,
    0b11001,
    0b10101,
    0b10011,
    0b10001,
    0b10001,
    0b10001
};

static const unsigned char FONT_O[7] = {
    0b01110,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b01110
};

static const unsigned char FONT_P[7] = {
    0b11110,
    0b10001,
    0b10001,
    0b11110,
    0b10000,
    0b10000,
    0b10000
};

static const unsigned char FONT_Q[7] = {
    0b01110,
    0b10001,
    0b10001,
    0b10001,
    0b10101,
    0b10010,
    0b01101
};

static const unsigned char FONT_R[7] = {
    0b11110,
    0b10001,
    0b10001,
    0b11110,
    0b10100,
    0b10010,
    0b10001
};

static const unsigned char FONT_S[7] = {
    0b01111,
    0b10000,
    0b10000,
    0b01110,
    0b00001,
    0b00001,
    0b11110
};

static const unsigned char FONT_T[7] = {
    0b11111,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100
};

static const unsigned char FONT_U[7] = {
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b01110
};

static const unsigned char FONT_V[7] = {
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b01010,
    0b00100
};

static const unsigned char FONT_W[7] = {
    0b10001,
    0b10001,
    0b10001,
    0b10101,
    0b10101,
    0b11011,
    0b10001
};

static const unsigned char FONT_X[7] = {
    0b10001,
    0b10001,
    0b01010,
    0b00100,
    0b01010,
    0b10001,
    0b10001
};

static const unsigned char FONT_Y[7] = {
    0b10001,
    0b10001,
    0b01010,
    0b00100,
    0b00100,
    0b00100,
    0b00100
};

static const unsigned char FONT_Z[7] = {
    0b11111,
    0b00001,
    0b00010,
    0b00100,
    0b01000,
    0b10000,
    0b11111
};


// ============================================================
// Symbols
// ============================================================

static const unsigned char FONT_SPACE[7] = {
    0, 0, 0, 0, 0, 0, 0
};

static const unsigned char FONT_EXCLAMATION[7] = {
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00000,
    0b00100
};

static const unsigned char FONT_QUOTE[7] = {
    0b01010,
    0b01010,
    0b01010,
    0,
    0,
    0,
    0
};

static const unsigned char FONT_HASH[7] = {
    0b01010,
    0b11111,
    0b01010,
    0b01010,
    0b11111,
    0b01010,
    0
};

static const unsigned char FONT_DOLLAR[7] = {
    0b00100,
    0b01111,
    0b10100,
    0b01110,
    0b00101,
    0b11110,
    0b00100
};

static const unsigned char FONT_PERCENT[7] = {
    0b11001,
    0b11010,
    0b00100,
    0b01000,
    0b10110,
    0b00110,
    0
};

static const unsigned char FONT_AMPERSAND[7] = {
    0b01100,
    0b10010,
    0b10100,
    0b01000,
    0b10101,
    0b10010,
    0b01101
};

static const unsigned char FONT_APOSTROPHE[7] = {
    0b00100,
    0b00100,
    0b01000,
    0,
    0,
    0,
    0
};

static const unsigned char FONT_LPAREN[7] = {
    0b00010,
    0b00100,
    0b01000,
    0b01000,
    0b01000,
    0b00100,
    0b00010
};

static const unsigned char FONT_RPAREN[7] = {
    0b01000,
    0b00100,
    0b00010,
    0b00010,
    0b00010,
    0b00100,
    0b01000
};

static const unsigned char FONT_STAR[7] = {
    0b00100,
    0b10101,
    0b01110,
    0b11111,
    0b01110,
    0b10101,
    0b00100
};

static const unsigned char FONT_PLUS[7] = {
    0b00100,
    0b00100,
    0b00100,
    0b11111,
    0b00100,
    0b00100,
    0b00100
};

static const unsigned char FONT_COMMA[7] = {
    0,
    0,
    0,
    0,
    0b00110,
    0b00100,
    0b01000
};

static const unsigned char FONT_MINUS[7] = {
    0,
    0,
    0,
    0b11111,
    0,
    0,
    0
};

static const unsigned char FONT_PERIOD[7] = {
    0,
    0,
    0,
    0,
    0,
    0b00110,
    0b00110
};

static const unsigned char FONT_SLASH[7] = {
    0b00001,
    0b00010,
    0b00100,
    0b01000,
    0b10000,
    0,
    0
};

static const unsigned char FONT_COLON[7] = {
    0,
    0b00110,
    0b00110,
    0,
    0,
    0b00110,
    0b00110
};

static const unsigned char FONT_SEMICOLON[7] = {
    0,
    0b00110,
    0b00110,
    0,
    0b00110,
    0b00100,
    0b01000
};

static const unsigned char FONT_LESS[7] = {
    0b00010,
    0b00100,
    0b01000,
    0b10000,
    0b01000,
    0b00100,
    0b00010
};

static const unsigned char FONT_EQUAL[7] = {
    0,
    0b11111,
    0,
    0b11111,
    0,
    0,
    0
};

static const unsigned char FONT_GREATER[7] = {
    0b01000,
    0b00100,
    0b00010,
    0b00001,
    0b00010,
    0b00100,
    0b01000
};

static const unsigned char FONT_QUESTION[7] = {
    0b01110,
    0b10001,
    0b00001,
    0b00010,
    0b00100,
    0,
    0b00100
};

static const unsigned char FONT_AT[7] = {
    0b01110,
    0b10001,
    0b10111,
    0b10101,
    0b10111,
    0b10000,
    0b01111
};

static const unsigned char FONT_LBRACKET[7] = {
    0b01110,
    0b01000,
    0b01000,
    0b01000,
    0b01000,
    0b01000,
    0b01110
};

static const unsigned char FONT_BACKSLASH[7] = {
    0b10000,
    0b01000,
    0b00100,
    0b00010,
    0b00001,
    0,
    0
};

static const unsigned char FONT_RBRACKET[7] = {
    0b01110,
    0b00010,
    0b00010,
    0b00010,
    0b00010,
    0b00010,
    0b01110
};

static const unsigned char FONT_CARET[7] = {
    0b00100,
    0b01010,
    0b10001,
    0,
    0,
    0,
    0
};

static const unsigned char FONT_UNDERSCORE[7] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0b11111
};

static const unsigned char FONT_BACKTICK[7] = {
    0b01000,
    0b00100,
    0,
    0,
    0,
    0,
    0
};

static const unsigned char FONT_LBRACE[7] = {
    0b00011,
    0b00100,
    0b00100,
    0b11000,
    0b00100,
    0b00100,
    0b00011
};

static const unsigned char FONT_PIPE[7] = {
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100
};

static const unsigned char FONT_RBRACE[7] = {
    0b11000,
    0b00100,
    0b00100,
    0b00011,
    0b00100,
    0b00100,
    0b11000
};

static const unsigned char FONT_TILDE[7] = {
    0,
    0b01001,
    0b10110,
    0,
    0,
    0,
    0
};




// ============================================================
// Get bitmap for a character
//
// This is the only public function in this file, so it must
// NOT be static. font.h declares it for other files to use.
//
// Unsupported characters return NULL and are simply skipped.
// ============================================================

const unsigned char *getGlyph(char c)
{
    // --------------------------------------------------------
    // Numbers
    // --------------------------------------------------------

    switch (c)
    {
        case '0': return FONT_0;
        case '1': return FONT_1;
        case '2': return FONT_2;
        case '3': return FONT_3;
        case '4': return FONT_4;
        case '5': return FONT_5;
        case '6': return FONT_6;
        case '7': return FONT_7;
        case '8': return FONT_8;
        case '9': return FONT_9;

        // ----------------------------------------------------
        // Lowercase
        // ----------------------------------------------------

        case 'a': return FONT_a;
        case 'b': return FONT_b;
        case 'c': return FONT_c;
        case 'd': return FONT_d;
        case 'e': return FONT_e;
        case 'f': return FONT_f;
        case 'g': return FONT_g;
        case 'h': return FONT_h;
        case 'i': return FONT_i;
        case 'j': return FONT_j;
        case 'k': return FONT_k;
        case 'l': return FONT_l;
        case 'm': return FONT_m;
        case 'n': return FONT_n;
        case 'o': return FONT_o;
        case 'p': return FONT_p;
        case 'q': return FONT_q;
        case 'r': return FONT_r;
        case 's': return FONT_s;
        case 't': return FONT_t;
        case 'u': return FONT_u;
        case 'v': return FONT_v;
        case 'w': return FONT_w;
        case 'x': return FONT_x;
        case 'y': return FONT_y;
        case 'z': return FONT_z;

        // ----------------------------------------------------
        // Uppercase
        // ----------------------------------------------------

        case 'A': return FONT_A;
        case 'B': return FONT_B;
        case 'C': return FONT_C;
        case 'D': return FONT_D;
        case 'E': return FONT_E;
        case 'F': return FONT_F;
        case 'G': return FONT_G;
        case 'H': return FONT_H;
        case 'I': return FONT_I;
        case 'J': return FONT_J;
        case 'K': return FONT_K;
        case 'L': return FONT_L;
        case 'M': return FONT_M;
        case 'N': return FONT_N;
        case 'O': return FONT_O;
        case 'P': return FONT_P;
        case 'Q': return FONT_Q;
        case 'R': return FONT_R;
        case 'S': return FONT_S;
        case 'T': return FONT_T;
        case 'U': return FONT_U;
        case 'V': return FONT_V;
        case 'W': return FONT_W;
        case 'X': return FONT_X;
        case 'Y': return FONT_Y;
        case 'Z': return FONT_Z;

        // ----------------------------------------------------
        // Symbols
        // ----------------------------------------------------

        case ' ': return FONT_SPACE;
        case '!': return FONT_EXCLAMATION;
        case '"': return FONT_QUOTE;
        case '#': return FONT_HASH;
        case '$': return FONT_DOLLAR;
        case '%': return FONT_PERCENT;
        case '&': return FONT_AMPERSAND;
        case '\'': return FONT_APOSTROPHE;
        case '(': return FONT_LPAREN;
        case ')': return FONT_RPAREN;
        case '*': return FONT_STAR;
        case '+': return FONT_PLUS;
        case ',': return FONT_COMMA;
        case '-': return FONT_MINUS;
        case '.': return FONT_PERIOD;
        case '/': return FONT_SLASH;
        case ':': return FONT_COLON;
        case ';': return FONT_SEMICOLON;
        case '<': return FONT_LESS;
        case '=': return FONT_EQUAL;
        case '>': return FONT_GREATER;
        case '?': return FONT_QUESTION;
        case '@': return FONT_AT;
        case '[': return FONT_LBRACKET;
        case '\\': return FONT_BACKSLASH;
        case ']': return FONT_RBRACKET;
        case '^': return FONT_CARET;
        case '_': return FONT_UNDERSCORE;
        case '`': return FONT_BACKTICK;
        case '{': return FONT_LBRACE;
        case '|': return FONT_PIPE;
        case '}': return FONT_RBRACE;
        case '~': return FONT_TILDE;

        default:
            return NULL;
    }
}