# 画像説明機能（ImageExplainMod）

## 概要
iPhoneなどのスマートフォンやPCから画像を送信すると、スタックチャンがその画像を説明してくれる機能です。

## 特徴
- 📱 **簡単な画像送信**: iPhoneのブラウザから直接画像をアップロード
- 🤖 **GPT-4 Vision統合**: OpenAIのGPT-4 Visionで画像を解析
- ⚡ **高速処理**: 最適化されたBase64エンコーディングとメモリ管理
- 🔄 **安定性重視**: エラーハンドリングとメモリ制限で安定動作

## 使い方

### 1. スタックチャンの起動
ファームウェアをビルドして起動すると、ImageExplainModが利用可能になります。

### 2. 画像説明モードに切り替え
- LCDを左右にフリックしてImageExplainModに切り替えます
- 画面に「画像説明モード」と表示されます

### 3. iPhoneから画像を送信

#### 方法1: Webインターフェース（推奨）
1. iPhoneのブラウザでスタックチャンのIPアドレスにアクセス
   ```
   http://<スタックチャンのIPアドレス>/image_upload_page
   ```
2. 「📷 画像を選択」ボタンをタップ、または画像をドラッグ&ドロップ
3. プレビューを確認
4. 「🚀 アップロード」ボタンをタップ

#### 方法2: curlコマンド（上級者向け）
```bash
curl -X POST -F "image=@/path/to/your/image.jpg" http://<IPアドレス>/image_upload
```

### 4. スタックチャンが説明
- 画像がアップロードされると自動的に解析が始まります
- 「画像を解析中...」→「AIに問い合わせ中...」と進行状況が表示されます
- GPT-4 Visionが画像を解析し、スタックチャンが説明を話します

### 5. 追加で質問する（会話継続）
画像の説明後、さらに質問できます：

#### 方法1: 画面タッチで音声入力
1. 画面上部をタッチ
2. 「御用でしょうか？」と聞かれたら質問を話す
   - 例: 「この写真はいつ撮られたものですか？」
   - 例: 「もっと詳しく説明してください」
   - 例: 「この色は何色ですか？」

#### 方法2: Webから質問付き画像を再送信
1. 新しい画像を選択
2. 質問欄に質問を入力
3. アップロード

#### 会話をリセット
- ボタンC（右下）を押すと会話履歴がクリアされます
- 新しい話題で再スタートできます

## ボタン操作

| ボタン | 機能 |
|--------|------|
| 画面上部タッチ | 音声入力で追加質問（会話継続モード時） |
| ボタンA（左下）| 最後の画像を再処理 |
| ボタンB（中央下）| 音声入力で追加質問（ボタンでも可能） |
| ボタンC（右下）| 会話履歴をクリア |

## 制限事項

### 画像サイズ
- **推奨**: 1MB以下
- **最大**: 2MB
- 2MBを超える画像はエラーになります

### 対応フォーマット
- JPEG (.jpg, .jpeg)
- PNG (.png)
- その他の一般的な画像フォーマット

### メモリ
- PSRAM（外部RAM）を使用して大きな画像も処理可能
- メモリ不足の場合はエラーメッセージが表示されます

## 技術詳細

### アーキテクチャ
```
iPhone/PC
    ↓ HTTP POST
WebServer (WebAPI.cpp)
    ↓ ファイル保存
SD Card (/app/AiStackChanEx/uploaded_image.jpg)
    ↓ 読み込み
ImageExplainMod
    ↓ Base64エンコード
Robot::chat() with base64_buf
    ↓
ChatGPT (GPT-4 Vision)
    ↓
TTS (音声合成)
    ↓
スタックチャンが説明
```

### 処理フロー
1. **アップロード**: WebサーバーがHTTP POSTで画像を受信
2. **保存**: SDカードに一時保存（`/app/AiStackChanEx/uploaded_image.jpg`）
3. **読み込み**: ImageExplainModが画像ファイルを読み込み
4. **エンコード**: Base64形式にエンコード（PSRAMを使用）
5. **AI解析**: GPT-4 VisionのAPIに送信
6. **音声合成**: レスポンスをTTSで音声化
7. **発話**: スタックチャンが説明

### 最適化ポイント

#### 速度向上
- PSRAMを使用して画像データを高速処理
- Base64エンコードを一度だけ実行
- 既存のRobot::chat()関数を再利用

#### 安定性確保
- ファイルサイズチェック（2MB制限）
- メモリ割り当てエラーのハンドリング
- アップロード中のステータス管理
- 処理中フラグでダブルクリック防止

## トラブルシューティング

### 画像がアップロードできない
- WiFi接続を確認
- IPアドレスが正しいか確認
- SDカードが正しくマウントされているか確認

### 「メモリ不足です」と表示される
- 画像サイズを小さくしてください（リサイズツールを使用）
- 他のアプリを終了して再起動

### 「画像ファイルが開けません」と表示される
- SDカードの空き容量を確認
- SDカードを再フォーマット（FAT32）

### AIが応答しない
- OpenAI APIキーが正しく設定されているか確認
- インターネット接続を確認
- シリアルモニタでエラーメッセージを確認

## 応答速度の最適化

### 高速化のヒント

#### 1. モデルの変更（最も効果的）
[ChatGPT.cpp](../firmware/src/llm/ChatGPT/ChatGPT.cpp)の`json_ChatString`を変更：
```cpp
// デフォルト（高品質）
String json_ChatString = 
"{\"model\": \"gpt-4o\","

// 高速・安価にする場合（推奨）
String json_ChatString = 
"{\"model\": \"gpt-4o-mini\","

// 最新モデル（GPT-5等、リリース後）
String json_ChatString = 
"{\"model\": \"gpt-5\","
// ※ モデル名はOpenAIの公式ドキュメントを参照してください
```

**比較**:
- `gpt-4o`: 高品質、やや遅い、高コスト
- `gpt-4o-mini`: 高速、安価、十分な品質（画像解析には推奨）
- `gpt-5`: （リリース時）最新モデル、契約プランにより利用可能

**新しいモデルの使用方法**:
1. OpenAIのアカウントで対象モデルへのアクセス権を確認
2. APIキーが該当モデルに対応しているか確認
3. 上記のコードで`model`フィールドを変更
4. ファームウェアを再ビルドして書き込み

#### 2. 画像サイズの最適化
アップロード前に画像をリサイズ：
- **推奨サイズ**: 800x600ピクセル以下
- **最大サイズ**: 1024x768ピクセル
- iPhoneの写真は4000x3000など大きいため、事前にリサイズすると大幅に高速化

リサイズ方法（iPhone）：
1. 写真アプリで編集
2. トリミング機能で小さく調整
3. または、画像圧縮アプリを使用

#### 3. WiFi環境の改善
- 5GHz帯を使用（2.4GHzより高速）
- スタックチャンをルーターの近くに配置
- 他のデバイスの通信を一時停止

#### 4. ストリーミング送信の調整
[ChatGPT.cpp](../firmware/src/llm/ChatGPT/ChatGPT.cpp)の`BufferedStreamingPrint`のバッファサイズを調整：
```cpp
// デフォルト
BufferedStreamingPrint stream(*client, 512);

// より大きなバッファ（高速化、メモリ消費増）
BufferedStreamingPrint stream(*client, 1024);
```

## カスタマイズ

### 質問文の変更
[ImageExplainMod.cpp](../firmware/src/mod/ImageExplain/ImageExplainMod.cpp)の以下の行を変更：

```cpp
robot->chat("この画像について詳しく説明してください。画像に質問や指示が書かれている場合は、それに答えてください。", base64Image.c_str());
```

例：
```cpp
// より詳細な説明を求める
robot->chat("この画像に何が写っていますか？色、形、位置なども詳しく教えてください。", base64Image.c_str());

// 特定の情報を抽出
robot->chat("この画像に写っている人物の人数と表情を教えてください。", base64Image.c_str());

// 子供向けの説明
robot->chat("3歳の子供にもわかるように、この画像を優しく説明してください。", base64Image.c_str());
```

### ファイル保存先の変更
[WebAPI.cpp](../firmware/src/WebAPI.cpp)の以下の行を変更：

```cpp
static String uploadPath = "/app/AiStackChanEx/uploaded_image.jpg";
```

### 画像サイズ制限の変更
[ImageExplainMod.cpp](../firmware/src/mod/ImageExplain/ImageExplainMod.cpp)の以下の行を変更：

```cpp
if(fileSize > 1024 * 1024 * 2) {  // 2MB以上
```

## 今後の拡張案
- [ ] カメラで直接撮影して説明
- [ ] 複数画像の連続処理
- [ ] 画像履歴の保存と再生
- [ ] OCR（文字認識）機能
- [ ] 物体検出とラベリング
- [ ] 画像比較機能

## 関連ファイル
- [ImageExplainMod.h](../firmware/src/mod/ImageExplain/ImageExplainMod.h)
- [ImageExplainMod.cpp](../firmware/src/mod/ImageExplain/ImageExplainMod.cpp)
- [WebAPI.cpp](../firmware/src/WebAPI.cpp)
- [ChatGPT.cpp](../firmware/src/llm/ChatGPT/ChatGPT.cpp)
