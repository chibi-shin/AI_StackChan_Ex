#ifndef _IMAGE_EXPLAIN_MOD_H
#define _IMAGE_EXPLAIN_MOD_H

#include <Arduino.h>
#include "mod/ModBase.h"

class ImageExplainMod: public ModBase{
private:
    box_t box_BtnA;
    box_t box_BtnB;
    box_t box_BtnC;
    box_t box_stt;  // 音声入力用
    
    bool processing;
    String lastImagePath;
    bool conversationMode;  // 会話継続モード
    
public:
    ImageExplainMod(bool _isOffline = false);
    
    void init(void);
    void pause(void);
    void btnA_pressed(void);
    void btnB_pressed(void);
    void btnC_pressed(void);
    void display_touched(int16_t x, int16_t y);
    void idle(void);
    
    void processImage(const String& imagePath);
};

#endif  //_IMAGE_EXPLAIN_MOD_H
