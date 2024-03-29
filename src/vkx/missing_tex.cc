#include <vkx/missing_tex.h>

unsigned char missingTex[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x01, 0x03, 0x00, 0x00, 0x00, 0xce, 0xb6, 0x46, 0xb9, 0x00, 0x00, 0x00,
    0x06, 0x50, 0x4c, 0x54, 0x45, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x9f,
    0xa6, 0x14, 0xf2, 0x00, 0x00, 0x00, 0x9c, 0x49, 0x44, 0x41, 0x54, 0x78,
    0xda, 0xed, 0xda, 0xa1, 0x11, 0x00, 0x30, 0x08, 0x45, 0xb1, 0xee, 0xbf,
    0x74, 0x31, 0xa0, 0x39, 0x24, 0x5c, 0x70, 0x98, 0x88, 0xaf, 0xdf, 0xab,
    0xfb, 0x79, 0xd3, 0xff, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x78, 0x00, 0x00, 0xa0, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x16,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xf4, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xf4, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x07, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xf4, 0x07, 0xdb, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x5c, 0x7d, 0xc3, 0xb2, 0xc7,
    0x06, 0x46, 0x95, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
    0x42, 0x60, 0x82};
unsigned int missingTex_len = 231;

StbImage &missingTexImage() {
  static StbImage image(missingTex, missingTex_len);
  return image;
}
