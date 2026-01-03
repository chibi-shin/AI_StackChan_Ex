#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#include "mod/ModManager.h"
#include "ImageExplainMod.h"
#include <Avatar.h>
#include "Robot.h"
#include <base64.h>

using namespace m5avatar;

/// 外部参照 ///
extern Avatar avatar;
extern Robot* robot;
extern bool servo_home;
extern void sw_tone();

// WebAPI.cppで定義されたグローバル変数
extern String g_uploadedImagePath;
extern bool g_imageUploaded;
extern String g_base64ImageBuffer;
///////////////

ImageExplainMod::ImageExplainMod(bool _isOffline)
  : processing{false}
{
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnB.setupBox(140, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);
}

void ImageExplainMod::init(void)
{
  avatar.setSpeechText("画像説明モード");
  avatar.setSpeechFont(&fonts::efontJA_12);
  delay(1000);
  avatar.setSpeechText("画像を送信してください");
  Serial.println("===========================================");
  Serial.println("Image Explain Mode");
  Serial.println("Send image via web interface");
  Serial.println("URL: http://<IP address>/image_upload");
  Serial.println("===========================================");
}

void ImageExplainMod::pause(void)
{
  avatar.setSpeechFont(&fonts::efontJA_16);
  avatar.setSpeechText("");
}

void ImageExplainMod::btnA_pressed(void)
{
  sw_tone();
  // 最後に受信した画像を再処理
  if(lastImagePath != "" && !processing){
    avatar.setSpeechText("再処理中...");
    processImage(lastImagePath);
  }
}

void ImageExplainMod::btnB_pressed(void)
{
  sw_tone();
  // カメラ機能がある場合はカメラ撮影も実装可能
  avatar.setSpeechText("未実装");
}

void ImageExplainMod::btnC_pressed(void)
{
  sw_tone();
  // 画像履歴のクリア
  lastImagePath = "";
  avatar.setSpeechText("履歴をクリア");
  delay(1000);
  avatar.setSpeechText("画像を送信してください");
}

void ImageExplainMod::display_touched(int16_t x, int16_t y)
{
  if (box_BtnA.contain(x, y))
  {
    btnA_pressed();
  }
  else if (box_BtnB.contain(x, y))
  {
    btnB_pressed();
  }
  else if (box_BtnC.contain(x, y))
  {
    btnC_pressed();
  }
}

void ImageExplainMod::idle(void)
{
  // アップロードされた画像があるかチェック
  if(g_imageUploaded && !processing) {
    g_imageUploaded = false;  // フラグをクリア
    Serial.println("New image detected, processing...");
    processImage(g_uploadedImagePath);
  }
}

void ImageExplainMod::processImage(const String& imagePath)
{
  if(processing){
    Serial.println("Already processing...");
    return;
  }
  
  processing = true;
  lastImagePath = imagePath;
  
  avatar.setExpression(Expression::Doubt);
  avatar.setSpeechText("画像を解析中...");
  
  Serial.println("Processing image: " + imagePath);
  
  // SDカードから画像を読み込む
  File imageFile = SD.open(imagePath.c_str(), FILE_READ);
  if (!imageFile) {
    Serial.println("Failed to open image file");
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("画像ファイルが開けません");
    processing = false;
    delay(2000);
    avatar.setSpeechText("画像を送信してください");
    avatar.setExpression(Expression::Neutral);
    return;
  }
  
  // ファイルサイズを取得
  size_t fileSize = imageFile.size();
  Serial.printf("Image file size: %d bytes\n", fileSize);
  
  // ファイルサイズチェック（大きすぎる場合はエラー）
  if(fileSize > 1024 * 1024 * 2) {  // 2MB以上
    Serial.println("Image file too large");
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("画像が大きすぎます");
    imageFile.close();
    processing = false;
    delay(2000);
    avatar.setSpeechText("画像を送信してください");
    avatar.setExpression(Expression::Neutral);
    return;
  }
  
  // 画像データをメモリに読み込む
  uint8_t* imageData = (uint8_t*)ps_malloc(fileSize);
  if(!imageData) {
    Serial.println("Failed to allocate memory for image");
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("メモリ不足です");
    imageFile.close();
    processing = false;
    delay(2000);
    avatar.setSpeechText("画像を送信してください");
    avatar.setExpression(Expression::Neutral);
    return;
  }
  
  size_t bytesRead = imageFile.read(imageData, fileSize);
  imageFile.close();
  
  if(bytesRead != fileSize) {
    Serial.println("Failed to read image file");
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("画像の読み込みエラー");
    free(imageData);
    processing = false;
    delay(2000);
    avatar.setSpeechText("画像を送信してください");
    avatar.setExpression(Expression::Neutral);
    return;
  }
  
  // Base64エンコード
  Serial.println("Encoding to Base64...");
  g_base64ImageBuffer = base64::encode(imageData, fileSize);
  free(imageData);
  
  Serial.printf("Base64 encoded size: %d bytes\n", g_base64ImageBuffer.length());
  
  // GPT-4 Visionに送信して解析
  avatar.setSpeechText("AIに問い合わせ中...");
  
  // Robotクラスのchat関数を使用（画像付き）
  // グローバルバッファを使用することでメモリコピーを避ける
  robot->chat("この画像について詳しく説明してください。", g_base64ImageBuffer.c_str());
  
  avatar.setExpression(Expression::Neutral);
  
  // Base64バッファをクリア（メモリ節約）
  g_base64ImageBuffer = "";
  
  processing = false;
  
  Serial.println("Image processing completed");
  delay(1000);
  avatar.setSpeechText("画像を送信してください");
}
