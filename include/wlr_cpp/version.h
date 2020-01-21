#ifdef __cplusplus
extern "C" {
#endif
#ifndef WLR_VERSION_H
#define WLR_VERSION_H

#define WLR_VERSION_STR "0.8.1"

#define WLR_VERSION_MAJOR 0
#define WLR_VERSION_MINOR 8
#define WLR_VERSION_MICRO 1

#define WLR_VERSION_NUM ((WLR_VERSION_MAJOR << 16) | (WLR_VERSION_MINOR << 8) | WLR_VERSION_MICRO)

#define WLR_VERSION_API_CURRENT 3
#define WLR_VERSION_API_REVISION 6
#define WLR_VERSION_API_AGE 1

#endif
#ifdef __cplusplus
}
#endif
