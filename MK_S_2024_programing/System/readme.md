# System  
## To Do List  
- ToDoはここに記すと管理しやすいかもしれない  
## Note  
- [MKS_all_code_ver2.ino](https://github.com/miku-gotoubun/RaSK/blob/main/MK_S_2024_programing/System/MKS_all_code_ver2.ino)  
これはロケット全体のプログラムを統合したものである．(SDカード以外)
ブザーがならない問題は修正済。

- [MKS_all_code__20240829_ver3.ino](https://github.com/miku-gotoubun/RaSK/blob/main/MK_S_2024_programing/System/MKS_all_code__20240829_ver3.ino)
  - SDカードにロケットの飛翔データ(９軸、気圧)のデータを書き込みすることができるようにした。
  - できればフライトピンが抜けた時間の記録、無線送信、パラシュート分離した時間の記録、無線送信を実装する。
## バージョン名の定義  
大型アップデートをしたらverが+1になる。

### 各バージョンの説明
|バージョン名|リリース日|説明|
|----|-----|-----|
|ver1|-----|三宅コードと統合、GPS動作不可能|
|ver2|2024-08-28|ロケット全体のプログラムを統合したものである．(SDカード以外) ブザーがならない問題を修正。|
|ver3|2024-08-29|SDカードにロケットの飛翔データ(９軸、気圧)のデータを書き込みを実装。|
|ver7|2024-09-02|温度測定と記録を実装、GPSの座標バグを修正、無線受信時の文字化けを修正。ほぼ文字化けが無くなりました。無線の通信速度を115200bpsから38400bpsに変更。|
