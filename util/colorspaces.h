#ifndef COLORSPACES_H
#define COLORSPACES_H


#include <stdint.h>


class ColorSpaces
{
public:

    static void hsv2rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);
};

#endif // COLORSPACES_H
