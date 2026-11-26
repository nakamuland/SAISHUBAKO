# 採集箱 / SAISHUBAKO

思いついたことを一行で残す。気になった音を録る。手元に置きたいメモを紙に出す。

初代 **M5Stack Cardputer** を、メモと音の採集機にするファームウェアです。
物理キーボードで英数字・ひらがな・カタカナを入力し、microSDへ保存します。
保存したメモは **MX11サーマルプリンターへBluetooth LEで直接印刷**できます。

## できること

- メモの入力・保存・一覧表示
- ローマ字からひらがな・カタカナへ変換
- 内蔵マイクで録音し、内蔵スピーカーで再生
- 録音に短いタイトルや説明を付ける
- メモの記録番号と本文をMX11で印刷
- microSDからTXT・WAVをPCへ取り出す

初代Cardputerと手元のMX11で、入力・保存・録音・再生・日本語入力・Fn＋Pによる印刷を確認しています。
他のMX11個体やCardputer-Advへの対応は未検証です。同じMX11という名称でも通信方式に違いがあります。

## 用意するもの

- 初代M5Stack Cardputer
- FAT32形式のmicroSDカード
- データ通信できるUSB-CケーブルとPC
- 印刷する場合はMX11、感熱紙

## すぐに書き込む

[書き込み用BINをダウンロード](https://github.com/nakamuland/SAISHUBAKO/raw/refs/heads/main/saishubako-cardputer-full.bin)し、Pythonとesptoolを使います。
ソース一式はGitHubの **Code → Download ZIP** から取得できます。

```powershell
python -m pip install esptool==4.7.0
```

1. CardputerをUSBでPCにつなぎます。
2. **G0を押し続けたままRSTを一度押し、1秒待ってG0を離します。** 書き込みモード中は画面が黒くなります。
3. デバイスマネージャーなどでCOMポートを確認します。以下の`COM17`は実際の番号に置き換えてください。
4. 既存ファームウェアを残したい場合は、その本体からバックアップを取ります。

```powershell
python -m esptool --chip esp32s3 --port COM17 --before no_reset --after no_reset read_flash 0x0 0x800000 cardputer.backup.bin
```

5. ダウンロードしたBINのあるフォルダで書き込みます。本体の既存ファームウェアを置き換える操作です。

```powershell
python -m esptool --chip esp32s3 --port COM17 --before no_reset --baud 460800 write_flash 0x0 saishubako-cardputer-full.bin
```

6. 完了後は **G0を押さず、RSTだけを一度押して**通常起動します。

Windowsでは同梱の`flash.ps1 -Port COM17`でも書き込めます。
macOS/Linuxではポート名を`/dev/cu.usbmodem...`や`/dev/ttyACM0`などに置き換えてください。
BINのSHA256は`SHA256SUMS.txt`に記載しています。
同梱BINは実機で確認した版と同じアプリソースから、開発PCのパスを置き換える設定で再ビルドしています。
公開用BINのビルドとイメージ検証を行っていますが、その再ビルド後のBIN自体は実機へ再書き込みしていません。

バックアップへ戻す場合は、再び書き込みモードにして実行します。

```powershell
python -m esptool --chip esp32s3 --port COM17 --before no_reset write_flash 0x0 cardputer.backup.bin
```

## 操作

| 画面 | 操作 | 動作 |
|---|---|---|
| メモ入力 | 文字キー | 文字を入力 |
| メモ入力 | Fn＋J | ABC → HIRA → KATAの順に入力モードを変更 |
| メモ入力 | Enter | メモを保存 |
| メモ入力 | Backspace/Del | 末尾の1文字を削除 |
| メモ入力 | Fn＋R またはG0 | 録音開始 |
| 録音中 | Fn＋R、Enter、G0 | 録音停止・保存 |
| 録音直後 | タイトルを入力してEnter | 今録った音声にテキストを添える |
| 録音直後 | 空のままEnter | タイトルを付けず次のメモへ |
| メモ入力 | Fn＋L | 保存済みの一覧を開く |
| 一覧 | `;` / `.` | 前／次の記録を選ぶ |
| 一覧 | Enter | 選んだ記録を開く |
| 記録の表示 | Enter | 録音を再生 |
| 再生中 | Enter / Tab / G0 | 再生停止 |
| 記録の表示 | Fn＋P | 記録番号とメモ本文をMX11へ印刷 |
| 記録の表示 | `;` / `.` | 長いメモの前／次のページ |
| 記録の表示 | Tab | 一覧へ戻る |
| 一覧 | Tab | 書きかけのメモへ戻る |

Fnは先に押してから対象のキーを押してください。
録音開始時に書きかけのメモがあれば、独立した記録として先に保存します。
音声への説明は録音を止めた後に入力してください。

### 日本語入力

- **HIRA**で`konnichiha` → `こんにちは`
- **KATA**で`kamera` → `カメラ`
- **KATA**で`ko-hi-` → `コーヒー`

基本的なローマ字変換です。漢字変換は未実装で、対応する綴りは`src/romaji.h`に定義しています。
入力中のローマ字は、保存や入力モード切り替え時に確定します。

### MX11印刷

1. MX11の電源を入れ、紙をセットします。
2. iPhoneなどの印刷アプリとの接続を切ります。接続できない場合は、iPhoneの設定でBluetoothをオフにしてMX11を再起動します。
3. 採集箱で**本文が表示される保存済みメモ**を開き、**Fn＋P**を押します。

BLEで`MX11`という名前を探します。実機で印刷できた`51 78`系のコマンド列をFn＋Pに割り当てています。
録音だけの記録に本文がなければ印刷できません。
画面の`Sent to MX11`は送信処理の完了を示すため、実際の印字は紙で確認してください。
印刷中はスキャン・接続・送信を行うため、操作完了まで待ちます。

## 保存形式

```text
SAISHU/
  00000001.txt    メモ
  00000002.wav    録音
  00000002.txt    録音のタイトル・説明（任意）
```

- テキストはUTF-8、1件あたり最大3072バイトです。
- 録音は16kHz・16bit・モノラルPCMのWAVで、1回あたり約30分まで。約1.9MB/分です。
- 記録は連番です。日時の記録は未実装です。
- 本体での保存済みメモの編集・削除は未実装です。PCで整理できます。
- 書きかけのメモはRAM上にあり、電源を切ると消えます。

録音・保存・印刷の終了を確認してから電源を切ってください。SDの挿し替えは電源を切って行います。
中断した録音の`.part`は次回起動時にWAVへ復旧を試みますが、電源断直前のデータやSD破損の復旧は保証できません。

## ソースからビルド

```powershell
python -m pip install platformio==6.1.19
python -m platformio run
```

`platformio.ini`で開発環境とライブラリの版を固定しています。
成功すると`package.py`がブートローダーとパーティション表を結合し、ルートに`saishubako-cardputer-full.bin`を生成します。
結合済みBINの書き込み先は`0x0`です。`.pio/build/cardputer/firmware.bin`はアプリ単体で`0x10000`用です。
付属の`build.ps1`でもビルドできます。新しくビルドしたBINのハッシュは同梱BINと異なる場合があります。

## ファイル構成

| ファイル | 内容 |
|---|---|
| `src/main.cpp` | 画面・入力・SD保存・録音・再生 |
| `src/romaji.h` | ローマ字変換とUTF-8処理 |
| `src/mx11.h` | MX11のBLE接続と画像送信 |
| `platformio.ini` | ビルド設定 |
| `package.py` | フルフラッシュイメージの生成 |
| `saishubako-cardputer-full.bin` | 実機で確認した版の書き込み用イメージ |

## ライセンス・参照

アプリのソースはMIT Licenseです。依存ライブラリとフォントは各作者のライセンスに従います。`THIRD_PARTY_NOTICES.md`を参照してください。

- [M5Cardputer](https://github.com/m5stack/M5Cardputer)
- [M5Unified](https://github.com/m5stack/M5Unified)
- [M5GFX](https://github.com/m5stack/M5GFX)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
- [MX11で使われたコマンド列](https://stackoverflow.com/questions/79301981/mx11-cat-thermal-printer-android-sdk)
- [FunPrint互換プロトコル実装](https://github.com/eraldonio/funprint)
- [MXW01の通信実測](https://github.com/eliorpom-cmd/print-on-my-desk)
