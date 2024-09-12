#ifndef PICOBIT_ESCAPES_H
#define PICOBIT_ESCAPES_H
#include <picobit.h>

#ifdef _cplusplus
extern "C" {
#endif

inline static uint8 schemeEscapeChar(const uint8 c, uint8** escape) {
    static const uint8 hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    static uint8 shortBuffer[] = {'\\', '\0', '\0'};
    static uint8 longBuffer[] = {'\\', 'x', '\0', '\0', '\0'};
    /*
    '((7 #\u0007 "7")
        (8 #\backspace "8")
        (9 #\tab "9")
        (10 #\newline "a")
        (11 #\vtab "b")
        (12 #\page "c")
        (13 #\return "d")
        (27 #\u001B "1b")
        (34 #\" "22")
        (92 #\\ "5c"))
    */
    switch (c) {
        case 0: __attribute__ ((fallthrough));
        case 1: __attribute__ ((fallthrough));
        case 2: __attribute__ ((fallthrough));
        case 3: __attribute__ ((fallthrough));
        case 4: __attribute__ ((fallthrough));
        case 5: __attribute__ ((fallthrough));
        case 6: 
            longBuffer[2] = '0';
            longBuffer[3] = hexDigits[c];
            *escape = longBuffer;
            return 0;
        case '\a':   // 07 bell / alert
            shortBuffer[1] = 'a';
            *escape = shortBuffer;
            return 0;
        case '\b':   // 08 backspace
            shortBuffer[1] = 'b';
            *escape = shortBuffer;
            return 0;
        case '\t':   // 09 horizontal tab
            shortBuffer[1] = 't';
            *escape = shortBuffer;
            return 0;
        case '\n':   // 0A new line
            shortBuffer[1] = 'n';
            *escape = shortBuffer;
            return 0;
        case '\v':   // 0B vertical tab
            shortBuffer[1] = 'v';
            *escape = shortBuffer;
            return 0;
        case '\f':   // 0C page break
            shortBuffer[1] = 'f';
            *escape = shortBuffer;
            return 0;
        case '\r':   // 0D carriage return
            shortBuffer[1] = 'r';
            *escape = shortBuffer;
            return 0;
        case '\x0E': /* 0xe */ __attribute__ ((fallthrough));
        case '\x0F': /* 0xf */
            // duplicate code is probably cheaper than second jump
            longBuffer[2] = '0';
            longBuffer[3] = hexDigits[c];
            *escape = longBuffer;
            return 0;
        case '\x10': /* 0x10 */ __attribute__ ((fallthrough));
        case '\x11': /* 0x11 */ __attribute__ ((fallthrough));
        case '\x12': /* 0x12 */ __attribute__ ((fallthrough));
        case '\x13': /* 0x13 */ __attribute__ ((fallthrough));
        case '\x14': /* 0x14 */ __attribute__ ((fallthrough));
        case '\x15': /* 0x15 */ __attribute__ ((fallthrough));
        case '\x16': /* 0x16 */ __attribute__ ((fallthrough));
        case '\x17': /* 0x17 */ __attribute__ ((fallthrough));
        case '\x18': /* 0x18 */ __attribute__ ((fallthrough));
        case '\x19': /* 0x19 */ __attribute__ ((fallthrough));
        case '\x1a': /* 0x1a */
            // duplicate code is probably cheaper than second jump
            longBuffer[2] = '1';
            longBuffer[3] = hexDigits[c & 0xf];
            *escape = longBuffer;
            return 0;
        case '\x1b': // 1B escape - don't use \e for C because it's non-standard, but Scheme knows it
            shortBuffer[1] = 'e';
            *escape = shortBuffer;
            return 0;
        case '\x1c': /* 0x1c */ __attribute__ ((fallthrough));
        case '\x1d': /* 0x1d */ __attribute__ ((fallthrough));
        case '\x1e': /* 0x1e */ __attribute__ ((fallthrough));
        case '\x1f': /* 0x1f */
            longBuffer[2] = '1';
            longBuffer[3] = hexDigits[c & 0xf];
            *escape = longBuffer;
            return 0;

        case ' ': /* 0x20 */ return c;
        case '!': /* 0x21 */ return c;
        case '"':    // 22 double quote
            shortBuffer[1] = '"';
            *escape = shortBuffer;
            return 0;
        case '#': /* 0x23 */ __attribute__ ((fallthrough));
        case '$': /* 0x24 */ __attribute__ ((fallthrough));
        case '%': /* 0x25 */ __attribute__ ((fallthrough));
        case '&': /* 0x26 */ __attribute__ ((fallthrough));
        case '\'': /*0x27 */ __attribute__ ((fallthrough));
        case '(': /* 0x28 */ __attribute__ ((fallthrough));
        case ')': /* 0x29 */ __attribute__ ((fallthrough));
        case '*': /* 0x2a */ __attribute__ ((fallthrough));
        case '+': /* 0x2b */ __attribute__ ((fallthrough));
        case ',': /* 0x2c */ __attribute__ ((fallthrough));
        case '-': /* 0x2d */ __attribute__ ((fallthrough));
        case '.': /* 0x2e */ __attribute__ ((fallthrough));
        case '/': /* 0x2f */ __attribute__ ((fallthrough));
        case '0': /* 0x30 */ __attribute__ ((fallthrough));
        case '1': /* 0x31 */ __attribute__ ((fallthrough));
        case '2': /* 0x32 */ __attribute__ ((fallthrough));
        case '3': /* 0x33 */ __attribute__ ((fallthrough));
        case '4': /* 0x34 */ __attribute__ ((fallthrough));
        case '5': /* 0x35 */ __attribute__ ((fallthrough));
        case '6': /* 0x36 */ __attribute__ ((fallthrough));
        case '7': /* 0x37 */ __attribute__ ((fallthrough));
        case '8': /* 0x38 */ __attribute__ ((fallthrough));
        case '9': /* 0x39 */ __attribute__ ((fallthrough));
        case ':': /* 0x3a */ __attribute__ ((fallthrough));
        case ';': /* 0x3b */ __attribute__ ((fallthrough));
        case '<': /* 0x3c */ __attribute__ ((fallthrough));
        case '=': /* 0x3d */ __attribute__ ((fallthrough));
        case '>': /* 0x3e */ __attribute__ ((fallthrough));
        case '?': /* 0x3f */ __attribute__ ((fallthrough));
        case '@': /* 0x40 */ __attribute__ ((fallthrough));
        case 'A': /* 0x41 */ __attribute__ ((fallthrough));
        case 'B': /* 0x42 */ __attribute__ ((fallthrough));
        case 'C': /* 0x43 */ __attribute__ ((fallthrough));
        case 'D': /* 0x44 */ __attribute__ ((fallthrough));
        case 'E': /* 0x45 */ __attribute__ ((fallthrough));
        case 'F': /* 0x46 */ __attribute__ ((fallthrough));
        case 'G': /* 0x47 */ __attribute__ ((fallthrough));
        case 'H': /* 0x48 */ __attribute__ ((fallthrough));
        case 'I': /* 0x49 */ __attribute__ ((fallthrough));
        case 'J': /* 0x4a */ __attribute__ ((fallthrough));
        case 'K': /* 0x4b */ __attribute__ ((fallthrough));
        case 'L': /* 0x4c */ __attribute__ ((fallthrough));
        case 'M': /* 0x4d */ __attribute__ ((fallthrough));
        case 'N': /* 0x4e */ __attribute__ ((fallthrough));
        case 'O': /* 0x4f */ __attribute__ ((fallthrough));
        case 'P': /* 0x50 */ __attribute__ ((fallthrough));
        case 'Q': /* 0x51 */ __attribute__ ((fallthrough));
        case 'R': /* 0x52 */ __attribute__ ((fallthrough));
        case 'S': /* 0x53 */ __attribute__ ((fallthrough));
        case 'T': /* 0x54 */ __attribute__ ((fallthrough));
        case 'U': /* 0x55 */ __attribute__ ((fallthrough));
        case 'V': /* 0x56 */ __attribute__ ((fallthrough));
        case 'W': /* 0x57 */ __attribute__ ((fallthrough));
        case 'X': /* 0x58 */ __attribute__ ((fallthrough));
        case 'Y': /* 0x59 */ __attribute__ ((fallthrough));
        case 'Z': /* 0x5a */ __attribute__ ((fallthrough));
        case '[': /* 0x5b */ return c;
        case '\\':   // 5C forward slash
            shortBuffer[1] = '\\';
            *escape = shortBuffer;
            return 0;
        case ']': /* 0x5d */ __attribute__ ((fallthrough));
        case '^': /* 0x5e */ __attribute__ ((fallthrough));
        case '_': /* 0x5f */ __attribute__ ((fallthrough));
        case '`': /* 0x60 */ __attribute__ ((fallthrough));
        case 'a': /* 0x61 */ __attribute__ ((fallthrough));
        case 'b': /* 0x62 */ __attribute__ ((fallthrough));
        case 'c': /* 0x63 */ __attribute__ ((fallthrough));
        case 'd': /* 0x64 */ __attribute__ ((fallthrough));
        case 'e': /* 0x65 */ __attribute__ ((fallthrough));
        case 'f': /* 0x66 */ __attribute__ ((fallthrough));
        case 'g': /* 0x67 */ __attribute__ ((fallthrough));
        case 'h': /* 0x68 */ __attribute__ ((fallthrough));
        case 'i': /* 0x69 */ __attribute__ ((fallthrough));
        case 'j': /* 0x6a */ __attribute__ ((fallthrough));
        case 'k': /* 0x6b */ __attribute__ ((fallthrough));
        case 'l': /* 0x6c */ __attribute__ ((fallthrough));
        case 'm': /* 0x6d */ __attribute__ ((fallthrough));
        case 'n': /* 0x6e */ __attribute__ ((fallthrough));
        case 'o': /* 0x6f */ __attribute__ ((fallthrough));
        case 'p': /* 0x70 */ __attribute__ ((fallthrough));
        case 'q': /* 0x71 */ __attribute__ ((fallthrough));
        case 'r': /* 0x72 */ __attribute__ ((fallthrough));
        case 's': /* 0x73 */ __attribute__ ((fallthrough));
        case 't': /* 0x74 */ __attribute__ ((fallthrough));
        case 'u': /* 0x75 */ __attribute__ ((fallthrough));
        case 'v': /* 0x76 */ __attribute__ ((fallthrough));
        case 'w': /* 0x77 */ __attribute__ ((fallthrough));
        case 'x': /* 0x78 */ __attribute__ ((fallthrough));
        case 'y': /* 0x79 */ __attribute__ ((fallthrough));
        case 'z': /* 0x7a */ __attribute__ ((fallthrough));
        case '{': /* 0x7b */ __attribute__ ((fallthrough));
        case '|': /* 0x7c */ __attribute__ ((fallthrough));
        case '}': /* 0x7d */ __attribute__ ((fallthrough));
        case '~': /* 0x7e */ return c;
        case '\x7f': /* 0x7f */
            longBuffer[2] = '7';
            longBuffer[3] = 'F';
            *escape = longBuffer;
            return 0;
        case (uint8)'\x80': /* 0x80 */ __attribute__ ((fallthrough));
        case (uint8)'\x81': /* 0x81 */ __attribute__ ((fallthrough));
        case (uint8)'\x82': /* 0x82 */ __attribute__ ((fallthrough));
        case (uint8)'\x83': /* 0x83 */ __attribute__ ((fallthrough));
        case (uint8)'\x84': /* 0x84 */ __attribute__ ((fallthrough));
        case (uint8)'\x85': /* 0x85 */ __attribute__ ((fallthrough));
        case (uint8)'\x86': /* 0x86 */ __attribute__ ((fallthrough));
        case (uint8)'\x87': /* 0x87 */ __attribute__ ((fallthrough));
        case (uint8)'\x88': /* 0x88 */ __attribute__ ((fallthrough));
        case (uint8)'\x89': /* 0x89 */ __attribute__ ((fallthrough));
        case (uint8)'\x8a': /* 0x8a */ __attribute__ ((fallthrough));
        case (uint8)'\x8b': /* 0x8b */ __attribute__ ((fallthrough));
        case (uint8)'\x8c': /* 0x8c */ __attribute__ ((fallthrough));
        case (uint8)'\x8d': /* 0x8d */ __attribute__ ((fallthrough));
        case (uint8)'\x8e': /* 0x8e */ __attribute__ ((fallthrough));
        case (uint8)'\x8f': /* 0x8f */
            longBuffer[2] = '8';
            longBuffer[3] = hexDigits[c & 0xf];
            *escape = longBuffer;
            return 0;
        case (uint8)'\x90': /* 0x90 */ __attribute__ ((fallthrough));
        case (uint8)'\x91': /* 0x91 */ __attribute__ ((fallthrough));
        case (uint8)'\x92': /* 0x92 */ __attribute__ ((fallthrough));
        case (uint8)'\x93': /* 0x93 */ __attribute__ ((fallthrough));
        case (uint8)'\x94': /* 0x94 */ __attribute__ ((fallthrough));
        case (uint8)'\x95': /* 0x95 */ __attribute__ ((fallthrough));
        case (uint8)'\x96': /* 0x96 */ __attribute__ ((fallthrough));
        case (uint8)'\x97': /* 0x97 */ __attribute__ ((fallthrough));
        case (uint8)'\x98': /* 0x98 */ __attribute__ ((fallthrough));
        case (uint8)'\x99': /* 0x99 */ __attribute__ ((fallthrough));
        case (uint8)'\x9a': /* 0x9a */ __attribute__ ((fallthrough));
        case (uint8)'\x9b': /* 0x9b */ __attribute__ ((fallthrough));
        case (uint8)'\x9c': /* 0x9c */ __attribute__ ((fallthrough));
        case (uint8)'\x9d': /* 0x9d */ __attribute__ ((fallthrough));
        case (uint8)'\x9e': /* 0x9e */ __attribute__ ((fallthrough));
        case (uint8)'\x9f': /* 0x9f */
            longBuffer[2] = '9';
            longBuffer[3] = hexDigits[c & 0xf];
            *escape = longBuffer;
            return 0;
        case (uint8)'\xa0': /* 0xa0 */ __attribute__ ((fallthrough));
        case (uint8)'\xa1': /* 0xa1 */ __attribute__ ((fallthrough));
        case (uint8)'\xa2': /* 0xa2 */ __attribute__ ((fallthrough));
        case (uint8)'\xa3': /* 0xa3 */ __attribute__ ((fallthrough));
        case (uint8)'\xa4': /* 0xa4 */ __attribute__ ((fallthrough));
        case (uint8)'\xa5': /* 0xa5 */ __attribute__ ((fallthrough));
        case (uint8)'\xa6': /* 0xa6 */ __attribute__ ((fallthrough));
        case (uint8)'\xa7': /* 0xa7 */ __attribute__ ((fallthrough));
        case (uint8)'\xa8': /* 0xa8 */ __attribute__ ((fallthrough));
        case (uint8)'\xa9': /* 0xa9 */ __attribute__ ((fallthrough));
        case (uint8)'\xaa': /* 0xaa */ __attribute__ ((fallthrough));
        case (uint8)'\xab': /* 0xab */ __attribute__ ((fallthrough));
        case (uint8)'\xac': /* 0xac */ __attribute__ ((fallthrough));
        case (uint8)'\xad': /* 0xad */ __attribute__ ((fallthrough));
        case (uint8)'\xae': /* 0xae */ __attribute__ ((fallthrough));
        case (uint8)'\xaf': /* 0xaf */
            longBuffer[2] = 'A';
            longBuffer[3] = hexDigits[c & 0xf];
            *escape = longBuffer;
            return 0;
        case (uint8)'\xb0': /* 0xb0 */ __attribute__ ((fallthrough));
        case (uint8)'\xb1': /* 0xb1 */ __attribute__ ((fallthrough));
        case (uint8)'\xb2': /* 0xb2 */ __attribute__ ((fallthrough));
        case (uint8)'\xb3': /* 0xb3 */ __attribute__ ((fallthrough));
        case (uint8)'\xb4': /* 0xb4 */ __attribute__ ((fallthrough));
        case (uint8)'\xb5': /* 0xb5 */ __attribute__ ((fallthrough));
        case (uint8)'\xb6': /* 0xb6 */ __attribute__ ((fallthrough));
        case (uint8)'\xb7': /* 0xb7 */ __attribute__ ((fallthrough));
        case (uint8)'\xb8': /* 0xb8 */ __attribute__ ((fallthrough));
        case (uint8)'\xb9': /* 0xb9 */ __attribute__ ((fallthrough));
        case (uint8)'\xba': /* 0xba */ __attribute__ ((fallthrough));
        case (uint8)'\xbb': /* 0xbb */ __attribute__ ((fallthrough));
        case (uint8)'\xbc': /* 0xbc */ __attribute__ ((fallthrough));
        case (uint8)'\xbd': /* 0xbd */ __attribute__ ((fallthrough));
        case (uint8)'\xbe': /* 0xbe */ __attribute__ ((fallthrough));
        case (uint8)'\xbf': /* 0xbf */
            longBuffer[2] = 'B';
            longBuffer[3] = hexDigits[c & 0xf];
            *escape = longBuffer;
            return 0;
        case (uint8)'\xc0': /* 0xc0 */ __attribute__ ((fallthrough));
        case (uint8)'\xc1': /* 0xc1 */ __attribute__ ((fallthrough));
        case (uint8)'\xc2': /* 0xc2 */ __attribute__ ((fallthrough));
        case (uint8)'\xc3': /* 0xc3 */ __attribute__ ((fallthrough));
        case (uint8)'\xc4': /* 0xc4 */ __attribute__ ((fallthrough));
        case (uint8)'\xc5': /* 0xc5 */ __attribute__ ((fallthrough));
        case (uint8)'\xc6': /* 0xc6 */ __attribute__ ((fallthrough));
        case (uint8)'\xc7': /* 0xc7 */ __attribute__ ((fallthrough));
        case (uint8)'\xc8': /* 0xc8 */ __attribute__ ((fallthrough));
        case (uint8)'\xc9': /* 0xc9 */ __attribute__ ((fallthrough));
        case (uint8)'\xca': /* 0xca */ __attribute__ ((fallthrough));
        case (uint8)'\xcb': /* 0xcb */ __attribute__ ((fallthrough));
        case (uint8)'\xcc': /* 0xcc */ __attribute__ ((fallthrough));
        case (uint8)'\xcd': /* 0xcd */ __attribute__ ((fallthrough));
        case (uint8)'\xce': /* 0xce */ __attribute__ ((fallthrough));
        case (uint8)'\xcf': /* 0xcf */
            longBuffer[2] = 'C';
            longBuffer[3] = hexDigits[c & 0xf];
            *escape = longBuffer;
            return 0;
        case (uint8)'\xd0': /* 0xd0 */ __attribute__ ((fallthrough));
        case (uint8)'\xd1': /* 0xd1 */ __attribute__ ((fallthrough));
        case (uint8)'\xd2': /* 0xd2 */ __attribute__ ((fallthrough));
        case (uint8)'\xd3': /* 0xd3 */ __attribute__ ((fallthrough));
        case (uint8)'\xd4': /* 0xd4 */ __attribute__ ((fallthrough));
        case (uint8)'\xd5': /* 0xd5 */ __attribute__ ((fallthrough));
        case (uint8)'\xd6': /* 0xd6 */ __attribute__ ((fallthrough));
        case (uint8)'\xd7': /* 0xd7 */ __attribute__ ((fallthrough));
        case (uint8)'\xd8': /* 0xd8 */ __attribute__ ((fallthrough));
        case (uint8)'\xd9': /* 0xd9 */ __attribute__ ((fallthrough));
        case (uint8)'\xda': /* 0xda */ __attribute__ ((fallthrough));
        case (uint8)'\xdb': /* 0xdb */ __attribute__ ((fallthrough));
        case (uint8)'\xdc': /* 0xdc */ __attribute__ ((fallthrough));
        case (uint8)'\xdd': /* 0xdd */ __attribute__ ((fallthrough));
        case (uint8)'\xde': /* 0xde */ __attribute__ ((fallthrough));
        case (uint8)'\xdf': /* 0xdf */
            longBuffer[2] = 'D';
            longBuffer[3] = hexDigits[c & 0xf];
            *escape = longBuffer;
            return 0;
        case (uint8)'\xe0': /* 0xe0 */ __attribute__ ((fallthrough));
        case (uint8)'\xe1': /* 0xe1 */ __attribute__ ((fallthrough));
        case (uint8)'\xe2': /* 0xe2 */ __attribute__ ((fallthrough));
        case (uint8)'\xe3': /* 0xe3 */ __attribute__ ((fallthrough));
        case (uint8)'\xe4': /* 0xe4 */ __attribute__ ((fallthrough));
        case (uint8)'\xe5': /* 0xe5 */ __attribute__ ((fallthrough));
        case (uint8)'\xe6': /* 0xe6 */ __attribute__ ((fallthrough));
        case (uint8)'\xe7': /* 0xe7 */ __attribute__ ((fallthrough));
        case (uint8)'\xe8': /* 0xe8 */ __attribute__ ((fallthrough));
        case (uint8)'\xe9': /* 0xe9 */ __attribute__ ((fallthrough));
        case (uint8)'\xea': /* 0xea */ __attribute__ ((fallthrough));
        case (uint8)'\xeb': /* 0xeb */ __attribute__ ((fallthrough));
        case (uint8)'\xec': /* 0xec */ __attribute__ ((fallthrough));
        case (uint8)'\xed': /* 0xed */ __attribute__ ((fallthrough));
        case (uint8)'\xee': /* 0xee */ __attribute__ ((fallthrough));
        case (uint8)'\xef': /* 0xef */
            longBuffer[2] = 'E';
            longBuffer[3] = hexDigits[c & 0xf];
            *escape = longBuffer;
            return 0;
        case (uint8)'\xf0': /* 0xf0 */ __attribute__ ((fallthrough));
        case (uint8)'\xf1': /* 0xf1 */ __attribute__ ((fallthrough));
        case (uint8)'\xf2': /* 0xf2 */ __attribute__ ((fallthrough));
        case (uint8)'\xf3': /* 0xf3 */ __attribute__ ((fallthrough));
        case (uint8)'\xf4': /* 0xf4 */ __attribute__ ((fallthrough));
        case (uint8)'\xf5': /* 0xf5 */ __attribute__ ((fallthrough));
        case (uint8)'\xf6': /* 0xf6 */ __attribute__ ((fallthrough));
        case (uint8)'\xf7': /* 0xf7 */ __attribute__ ((fallthrough));
        case (uint8)'\xf8': /* 0xf8 */ __attribute__ ((fallthrough));
        case (uint8)'\xf9': /* 0xf9 */ __attribute__ ((fallthrough));
        case (uint8)'\xfa': /* 0xfa */ __attribute__ ((fallthrough));
        case (uint8)'\xfb': /* 0xfb */ __attribute__ ((fallthrough));
        case (uint8)'\xfc': /* 0xfc */ __attribute__ ((fallthrough));
        case (uint8)'\xfd': /* 0xfd */ __attribute__ ((fallthrough));
        case (uint8)'\xfe': /* 0xfe */ __attribute__ ((fallthrough));
        case (uint8)'\xff': /* 0xff */
            longBuffer[2] = 'F';
            longBuffer[3] = hexDigits[c & 0xf];
            *escape = longBuffer;
            return 0;
    }
    // some int conversion chicanery is causing the compiler to think there are non-returning codepaths?
    ERROR("schemeEscapeChar", "logic error: switch should be exhaustive");
    (__builtin_unreachable());
}

#ifdef _cplusplus
}
#endif

#endif
