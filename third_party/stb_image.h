// 单文件头库 stb_image.h - https://github.com/nothings/stb
#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char stbi_uc;

stbi_uc *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);
void stbi_image_free(void *retval);
void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);

#ifdef __cplusplus
}
#endif

#endif // STB_IMAGE_H
