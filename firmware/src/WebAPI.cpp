#include <ESP32WebServer.h>
#include <nvs.h>
#include <SD.h>
#include "WebAPI.h"
#include "Avatar.h"
#include "llm/ChatGPT/ChatGPT.h"
#include "llm/ChatGPT/FunctionCall.h"
#include "Robot.h"
#include "mod/ModManager.h"
#include "mod/ImageExplain/ImageExplainMod.h"

using namespace m5avatar;
extern Avatar avatar;
extern uint8_t m5spk_virtual_channel;
extern String STT_API_KEY;

ESP32WebServer server(80);

// 画像アップロード用のグローバル変数
String g_uploadedImagePath = "";
bool g_imageUploaded = false;
String g_base64ImageBuffer = "";  // Base64エンコードされた画像データ
String g_imageQuestion = "";     // 画像に対する質問文

// C++11 multiline string constants are neato...
static const char HEAD[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html lang="ja">
<head>
  <meta charset="UTF-8">
  <title>AIｽﾀｯｸﾁｬﾝ</title>
</head>)KEWL";

static const char APIKEY_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>APIキー設定</title>
  </head>
  <body>
    <h1>APIキー設定</h1>
    <form>
      <label for="role1">OpenAI API Key</label>
      <input type="text" id="openai" name="openai" oninput="adjustSize(this)"><br>
      <label for="role2">VoiceVox API Key</label>
      <input type="text" id="voicevox" name="voicevox" oninput="adjustSize(this)"><br>
      <label for="role3">Speech to Text API Key</label>
      <input type="text" id="sttapikey" name="sttapikey" oninput="adjustSize(this)"><br>
      <button type="button" onclick="sendData()">送信する</button>
    </form>
    <script>
      function adjustSize(input) {
        input.style.width = ((input.value.length + 1) * 8) + 'px';
      }
      function sendData() {
        // FormDataオブジェクトを作成
        const formData = new FormData();

        // 各ロールの値をFormDataオブジェクトに追加
        const openaiValue = document.getElementById("openai").value;
        if (openaiValue !== "") formData.append("openai", openaiValue);

        const voicevoxValue = document.getElementById("voicevox").value;
        if (voicevoxValue !== "") formData.append("voicevox", voicevoxValue);

        const sttapikeyValue = document.getElementById("sttapikey").value;
        if (sttapikeyValue !== "") formData.append("sttapikey", sttapikeyValue);

	    // POSTリクエストを送信
	    const xhr = new XMLHttpRequest();
	    xhr.open("POST", "/apikey_set");
	    xhr.onload = function() {
	      if (xhr.status === 200) {
	        alert("データを送信しました！");
	      } else {
	        alert("送信に失敗しました。");
	      }
	    };
	    xhr.send(formData);
	  }
	</script>
  </body>
</html>)KEWL";

static const char ROLE_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
<head>
	<title>ロール設定</title>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<style>
		textarea {
			width: 80%;
			height: 200px;
			resize: both;
		}
	</style>
</head>
<body>
	<h1>ロール設定</h1>
	<form onsubmit="postData(event)">
		<label for="textarea">ここにロールを記述してください。:</label><br>
		<textarea id="textarea" name="textarea"></textarea><br><br>
		<input type="submit" value="Submit">
	</form>
	<script>
		function postData(event) {
			event.preventDefault();
			const textAreaContent = document.getElementById("textarea").value.trim();
//			if (textAreaContent.length > 0) {
				const xhr = new XMLHttpRequest();
				xhr.open("POST", "/role_set", true);
				xhr.setRequestHeader("Content-Type", "text/plain;charset=UTF-8");
			// xhr.onload = () => {
			// 	location.reload(); // 送信後にページをリロード
			// };
			xhr.onload = () => {
				document.open();
				document.write(xhr.responseText);
				document.close();
			};
				xhr.send(textAreaContent);
//        document.getElementById("textarea").value = "";
				alert("Data sent successfully!");
//			} else {
//				alert("Please enter some text before submitting.");
//			}
		}
	</script>
</body>
</html>)KEWL";

static const char IMAGE_UPLOAD_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
<head>
	<title>画像アップロード</title>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<style>
		body {
			font-family: Arial, sans-serif;
			max-width: 600px;
			margin: 50px auto;
			padding: 20px;
		}
		.upload-area {
			border: 2px dashed #ccc;
			border-radius: 10px;
			padding: 40px;
			text-align: center;
			margin: 20px 0;
		}
		#preview {
			max-width: 100%;
			max-height: 400px;
			margin: 20px 0;
			display: none;
		}
		button {
			background-color: #4CAF50;
			color: white;
			padding: 15px 32px;
			font-size: 16px;
			border: none;
			border-radius: 4px;
			cursor: pointer;
			margin: 10px;
		}
		button:hover {
			background-color: #45a049;
		}
		button:disabled {
			background-color: #cccccc;
			cursor: not-allowed;
		}
		#status {
			margin: 20px 0;
			padding: 10px;
			border-radius: 4px;
		}
		.success {
			background-color: #d4edda;
			color: #155724;
		}
		.error {
			background-color: #f8d7da;
			color: #721c24;
		}
		.info {
			background-color: #d1ecf1;
			color: #0c5460;
		}
	</style>
</head>
<body>
	<h1>🤖 スタックチャン画像説明</h1>
	<p>画像を選択してアップロードすると、スタックチャンが説明してくれます</p>
	
	<div class="upload-area">
		<input type="file" id="imageInput" accept="image/*" style="display: none;">
		<button onclick="document.getElementById('imageInput').click()">📷 画像を選択</button>
		<p>または、ここに画像をドロップ</p>
	</div>
	
	<img id="preview" alt="プレビュー">
	
	<div style="margin: 20px 0;">
		<label for="questionInput" style="display: block; margin-bottom: 10px; font-weight: bold;">💬 質問（オプション）:</label>
		<textarea id="questionInput" placeholder="画像について質問がある場合は入力してください（例: この写真に写っている物は何ですか？）" style="width: 100%; height: 80px; padding: 10px; border: 1px solid #ccc; border-radius: 4px; font-size: 14px; box-sizing: border-box;"></textarea>
	</div>
	
	<div style="text-align: center;">
		<button id="uploadBtn" onclick="uploadImage()" disabled>🚀 アップロード</button>
		<button onclick="clearImage()">🗑️ クリア</button>
	</div>
	
	<div id="status"></div>
	
	<script>
		let selectedFile = null;
		
		const imageInput = document.getElementById('imageInput');
		const preview = document.getElementById('preview');
		const uploadBtn = document.getElementById('uploadBtn');
		const status = document.getElementById('status');
		const uploadArea = document.querySelector('.upload-area');
		
		// ファイル選択時
		imageInput.addEventListener('change', function(e) {
			const file = e.target.files[0];
			if (file) {
				handleFile(file);
			}
		});
		
		// ドラッグ&ドロップ
		uploadArea.addEventListener('dragover', function(e) {
			e.preventDefault();
			uploadArea.style.borderColor = '#4CAF50';
		});
		
		uploadArea.addEventListener('dragleave', function(e) {
			uploadArea.style.borderColor = '#ccc';
		});
		
		uploadArea.addEventListener('drop', function(e) {
			e.preventDefault();
			uploadArea.style.borderColor = '#ccc';
			const file = e.dataTransfer.files[0];
			if (file && file.type.startsWith('image/')) {
				handleFile(file);
			}
		});
		
		function handleFile(file) {
			// ファイルサイズチェック（2MB以下）
			if (file.size > 2 * 1024 * 1024) {
				showStatus('画像サイズは2MB以下にしてください', 'error');
				return;
			}
			
			selectedFile = file;
			
			// プレビュー表示
			const reader = new FileReader();
			reader.onload = function(e) {
				preview.src = e.target.result;
				preview.style.display = 'block';
				uploadBtn.disabled = false;
				showStatus('画像を選択しました。アップロードボタンを押してください。', 'info');
			};
			reader.readAsDataURL(file);
		}
		
		function uploadImage() {
			if (!selectedFile) {
				showStatus('画像を選択してください', 'error');
				return;
			}
			
			uploadBtn.disabled = true;
			showStatus('アップロード中...', 'info');
			
			const formData = new FormData();
			formData.append('image', selectedFile);
			
			// 質問文があれば追加
			const question = document.getElementById('questionInput').value.trim();
			if (question) {
				formData.append('question', question);
			}
			
			fetch('/image_upload', {
				method: 'POST',
				body: formData
			})
			.then(response => response.text())
			.then(data => {
				showStatus('アップロード成功！スタックチャンが画像を説明します。', 'success');
				uploadBtn.disabled = false;
			})
			.catch(error => {
				showStatus('アップロード失敗: ' + error, 'error');
				uploadBtn.disabled = false;
			});
		}
		
		function clearImage() {
			selectedFile = null;
			document.getElementById('questionInput').value = '';
			preview.style.display = 'none';
			preview.src = '';
			imageInput.value = '';
			uploadBtn.disabled = true;
			status.innerHTML = '';
		}
		
		function showStatus(message, type) {
			status.innerHTML = message;
			status.className = type;
		}
	</script>
</body>
</html>)KEWL";


void handleRoot() {
  server.send(200, "text/plain", "hello from m5stack!");
}


void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
//  server.send(404, "text/plain", message);
  server.send(404, "text/html", String(HEAD) + String("<body>") + message + String("</body>"));
}

void handle_speech() {
  String message = server.arg("say");
  String speaker = server.arg("voice");
  //if(speaker != "") {
  //  TTS_PARMS = TTS_SPEAKER + speaker;
  //}
  Serial.println(message);
  ////////////////////////////////////////
  // 音声の発声
  ////////////////////////////////////////
  //avatar.setExpression(Expression::Happy);
  robot->speech(message);
  server.send(200, "text/plain", String("OK"));
}

void handle_chat() {
  static String response = "";
  // tts_parms_no = 1;
  String text = server.arg("text");
  String speaker = server.arg("voice");
  //if(speaker != "") {
  //  TTS_PARMS = TTS_SPEAKER + speaker;
  //}

  robot->chat(text);

  server.send(200, "text/html", String(HEAD)+String("<body>")+response+String("</body>"));
}

void handle_apikey() {
  // ファイルを読み込み、クライアントに送信する
  server.send(200, "text/html", APIKEY_HTML);
}

#if 0
void handle_apikey_set() {
  // POST以外は拒否
  if (server.method() != HTTP_POST) {
    return;
  }
  // openai
  String openai = server.arg("openai");
  // voicetxt
  String voicevox = server.arg("voicevox");
  // voicetxt
  String sttapikey = server.arg("sttapikey");
 
  OPENAI_API_KEY = openai;
  VOICEVOX_API_KEY = voicevox;
  STT_API_KEY = sttapikey;
  Serial.println(openai);
  Serial.println(voicevox);
  Serial.println(sttapikey);

  uint32_t nvs_handle;
  if (ESP_OK == nvs_open("apikey", NVS_READWRITE, &nvs_handle)) {
    nvs_set_str(nvs_handle, "openai", openai.c_str());
    nvs_set_str(nvs_handle, "voicevox", voicevox.c_str());
    nvs_set_str(nvs_handle, "sttapikey", sttapikey.c_str());
    nvs_close(nvs_handle);
  }
  server.send(200, "text/plain", String("OK"));
}
#endif

void handle_role() {
  // ファイルを読み込み、クライアントに送信する
  server.send(200, "text/html", ROLE_HTML);
}


/**
 * アプリからテキスト(文字列)と共にRoll情報が配列でPOSTされてくることを想定してJSONを扱いやすい形に変更
 * 出力形式をJSONに変更
*/
void handle_role_set() {

  // ModuleLLMのLLMを使用している場合はロール設定は不可
  if(robot->m_config.getExConfig().llm.type == LLM_TYPE_MODULE_LLM){
    return;
  }

  // POST以外は拒否
  if (server.method() != HTTP_POST) {
    return;
  }
  String role = server.arg("plain");
  if (role != "") {
//    init_chat_doc(InitBuffer.c_str());
    robot->llm->init_chat_doc(json_ChatString.c_str());
    JsonArray messages = chat_doc["messages"];
    JsonObject systemMessage1 = messages.createNestedObject();
    systemMessage1["role"] = "system";
    systemMessage1["content"] = role;
//    serializeJson(chat_doc, InitBuffer);
  } else {
    robot->llm->init_chat_doc(json_ChatString.c_str());
  }
  //会話履歴をクリア
  chatHistory.clear();

#if 0  //save_role()に移動
  InitBuffer="";
  serializeJson(chat_doc, InitBuffer);
  Serial.println("InitBuffer = " + InitBuffer);
  //Role_JSON = InitBuffer;
#endif

  // JSONデータをspiffsへ出力する
  robot->llm->save_role();

  // 整形したJSONデータを出力するHTMLデータを作成する
  String html = "<html><body><pre>";
  serializeJsonPretty(chat_doc, html);
  html += "</pre></body></html>";

  // HTMLデータをシリアルに出力する
  Serial.println(html);
  server.send(200, "text/html", html);
//  server.send(200, "text/plain", String("OK"));
};

// 整形したJSONデータを出力するHTMLデータを作成する
void handle_role_get() {

  String html = "<html><body><pre>";
  serializeJsonPretty(chat_doc, html);
  html += "</pre></body></html>";

  // HTMLデータをシリアルに出力する
  Serial.println(html);
  server.send(200, "text/html", String(HEAD) + html);
};

void handle_image_upload_page() {
  server.send(200, "text/html", IMAGE_UPLOAD_HTML);
}

void handle_image_upload() {
  // POST以外は拒否
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  HTTPUpload& upload = server.upload();
  static File uploadFile;
  static String uploadPath = "/app/AiStackChanEx/uploaded_image.jpg";

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Upload Start: %s\n", upload.filename.c_str());
    
    // SDカードの初期化確認
    if(!SD.begin(GPIO_NUM_4, SPI, 25000000)) {
      Serial.println("SD Card Mount Failed");
      server.send(500, "text/plain", "SD Card Error");
      return;
    }

    // アップロードディレクトリの作成
    if(!SD.exists("/app/AiStackChanEx")) {
      SD.mkdir("/app");
      SD.mkdir("/app/AiStackChanEx");
    }

    // ファイルを開く（上書き）
    uploadFile = SD.open(uploadPath.c_str(), FILE_WRITE);
    if (!uploadFile) {
      Serial.println("Failed to open file for writing");
      server.send(500, "text/plain", "File Open Error");
      return;
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    // データを書き込む
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
      Serial.printf("Writing: %d bytes\n", upload.currentSize);
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.printf("Upload Complete: %d bytes\n", upload.totalSize);
      
      // 質問文を取得（POSTパラメータから）
      if (server.hasArg("question")) {
        g_imageQuestion = server.arg("question");
        Serial.println("Question: " + g_imageQuestion);
      } else {
        g_imageQuestion = "";
      }
      
      // グローバル変数に画像パスを保存
      g_uploadedImagePath = uploadPath;
      g_imageUploaded = true;
      
      server.send(200, "text/plain", "OK - Image uploaded successfully");
      Serial.println("Image uploaded: " + uploadPath);
    } else {
      server.send(500, "text/plain", "Upload Error");
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile) {
      uploadFile.close();
    }
    Serial.println("Upload Aborted");
    server.send(500, "text/plain", "Upload Aborted");
  }
}

void handle_face() {
  String expression = server.arg("expression");
  expression = expression + "\n";
  Serial.println(expression);
  switch (expression.toInt())
  {
    case 0: avatar.setExpression(Expression::Neutral); break;
    case 1: avatar.setExpression(Expression::Happy); break;
    case 2: avatar.setExpression(Expression::Sleepy); break;
    case 3: avatar.setExpression(Expression::Doubt); break;
    case 4: avatar.setExpression(Expression::Sad); break;
    case 5: avatar.setExpression(Expression::Angry); break;  
  } 
  server.send(200, "text/plain", String("OK"));
}

#if 0
void handle_setting() {
  String value = server.arg("volume");
  String led = server.arg("led");
  String speaker = server.arg("speaker");
//  volume = volume + "\n";
  Serial.println(speaker);
  Serial.println(value);
  size_t speaker_no;

  if(speaker != ""){
    speaker_no = speaker.toInt();
    if(speaker_no > 60) {
      speaker_no = 60;
    }
    TTS_SPEAKER_NO = String(speaker_no);
    TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;
  }

  if(value == "") value = "180";
  size_t volume = value.toInt();
  uint8_t led_onoff = 0;
  uint32_t nvs_handle;
  if (ESP_OK == nvs_open("setting", NVS_READWRITE, &nvs_handle)) {
    if(volume > 255) volume = 255;
    nvs_set_u32(nvs_handle, "volume", volume);
    if(led != "") {
      if(led == "on") led_onoff = 1;
      else  led_onoff = 0;
      nvs_set_u8(nvs_handle, "led", led_onoff);
    }
    nvs_set_u8(nvs_handle, "speaker", speaker_no);

    nvs_close(nvs_handle);
  }
  M5.Speaker.setVolume(volume);
  M5.Speaker.setChannelVolume(m5spk_virtual_channel, volume);
  server.send(200, "text/plain", String("OK"));
}
#endif

void init_web_server(void)
{

  server.on("/", handleRoot);
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  // And as regular external functions:
  server.on("/speech", handle_speech);
  server.on("/face", handle_face);
  server.on("/chat", handle_chat);
  server.on("/apikey", handle_apikey);
  //server.on("/setting", handle_setting);
  //server.on("/apikey_set", HTTP_POST, handle_apikey_set);
  server.on("/role", handle_role);
  server.on("/role_set", HTTP_POST, handle_role_set);
  server.on("/role_get", handle_role_get);
  server.on("/image_upload_page", handle_image_upload_page);
  server.on("/image_upload", HTTP_POST, handle_image_upload, handle_image_upload);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  M5.Lcd.println("HTTP server started");  
}

void web_server_handle_client(void)
{
  server.handleClient();
}
