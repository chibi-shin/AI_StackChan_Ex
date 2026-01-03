#ifndef _WEB_API_H
#define _WEB_API_H

#include <Arduino.h>
#include <M5Unified.h>

extern void init_web_server(void);
extern void web_server_handle_client(void);

// 画像アップロード用のグローバル変数
extern String g_uploadedImagePath;
extern bool g_imageUploaded;
extern String g_base64ImageBuffer;

#endif  //_WEB_API_H