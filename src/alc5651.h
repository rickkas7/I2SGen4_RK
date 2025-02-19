#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void alc5651_init(void);
void alc5651_init_interface2(void);
void alc5651_reg_dump(void);
void alc5651_index_dump(void);
void alc5651_set_word_len(int len_idx);


#ifdef __cplusplus
}
#endif
