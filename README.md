# ArUco 3MFメーカー

このアプリは OpenSiv3D を使って ArUco マーカーの多色 3MF を生成します。
OpenSiv3D は Git サブモジュールとして管理します。
再利用する部品や、単体では実行しない関数は `src/` に置きます。
実際に起動して動作を確認するプログラムは `functional_test/` に置きます。
SDK は `external/OpenSiv3D` の Git サブモジュールから共有します。

## src と functional_test の使い分け

- `src/`: 他のプログラムから呼び出す関数、クラス、共通処理を置きます。
    ここに置くファイルは、通常は単体で実行しません。
- `functional_test/`: `src/` の部品を組み合わせ、実際に起動して動作を確認する
    プログラムを置きます。画面表示、手動確認、機能の組み合わせテストなどが対象です。
- `build/`: CMake が生成するビルドファイルと実行ファイルを置きます。直接編集しません。

## セットアップ

Ubuntu/Debian では、最初にビルドに必要なシステムパッケージをインストールします。
このスクリプトは `sudo` を使って apt パッケージを追加します。

```bash
./scripts/install_dependencies.sh
```

続けてサブモジュールを取得し、ビルドします。

```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

Siv3D が `resources` ディレクトリを見つけられるように、ビルド後の実行ファイルを
起動します。

```bash
./build/bin/siv3d_app
```

## ArUco マーカーを3MFで保存する

アプリで辞書、マーカーID、マーカーの幅、白い土台の厚さ、黒い模様の高さを指定し、
「3MFを書き出す」を押して保存先を選びます。対応辞書とID範囲は次のとおりです。
入力欄は Tab で次へ、Shift+Tab で前へ移動し、最後と最初の欄の間は循環します。
IDは半角数字のみ、寸法と厚さは半角数字とドットのみを入力できます。

| 辞書                               | ID      |
| ---------------------------------- | ------- |
| ArUco 4x4 / 5x5 / 6x6 / 7x7 (1000) | 0〜999  |
| MIP_36h12                          | 0〜249  |
| AprilTag 16h5                      | 0〜29   |
| AprilTag 25h9                      | 0〜34   |
| AprilTag 36h10                     | 0〜2319 |
| AprilTag 36h11                     | 0〜586  |

サイズの基準は「マーカー自体」（黒い外枠を含む幅、初期値）と「白い余白を含む」
（プレート全体の幅）から選択できます。基準を切り替えても入力値は変わらず、
選択した基準の幅として扱います。外側に1セル分の白い余白を付けるため、
両者の比率は辞書のビットサイズに応じて変わり、例えば4x4辞書ならプレート幅は
マーカー幅の4/3です。「各辺の追加余白 (mm)」で、この1セル分の余白の外側に
追加する幅を指定できます（初期値は0 mm）。指定した追加余白はプレート全体の幅にも反映されます。
プレビュー下の両矢印にマーカー幅とプレート幅を表示します。

出力される3MFには白い土台と黒い模様が別の色として格納されます。スライサーで
2色のフィラメントを割り当てて印刷してください。複数色の割り当てはスライサー側の
設定が必要です。
保存時の拡張子は `.3mf` に統一します。3MFは内部的にZIPパッケージを使うため、
ファイルマネージャーがZIPとして表示する場合がありますが、スライサーでは3MFとして開けます。

ArUco と AprilTag のコードは OpenCV の predefined dictionaries を使用します。
MIP_36h12 のコード表も OpenCV の
[predefined_dictionaries.hpp](https://github.com/opencv/opencv/blob/4.x/modules/objdetect/src/aruco/predefined_dictionaries.hpp)
から取得しています。OpenCV のライセンスは同リポジトリの
[LICENSE](https://github.com/opencv/opencv/blob/4.x/LICENSE) を参照してください。

## Web版のビルド

Web版はブラウザー内で3MFを生成し、ダウンロードします。Linux版の保存ダイアログは
変更されません。[Siv3D for Web の手順](https://siv3d.kamenokosoft.com/docs/ja/download/web-vscode-unix/)
にあるテンプレート（同梱SDKはv0.6.6）と Emscripten 3.1.20 を使用します。
テンプレート付属のライブラリの未解決シンボルを避けるため、同じバージョンの
FreeType と OpenCV を WebAssembly 用に再ビルドします。次の例では Python を uv で用意します。

```bash
git submodule update --init --recursive
mkdir -p build/web-tools
uv venv --managed-python --python 3.11 build/web-tools/python
source build/web-tools/python/bin/activate
git clone https://github.com/emscripten-core/emsdk.git build/web-tools/emsdk
build/web-tools/emsdk/emsdk install 3.1.20
build/web-tools/emsdk/emsdk activate 3.1.20
source build/web-tools/emsdk/emsdk_env.sh
git clone https://github.com/nokotan/OpenSiv3DForWeb-VSCode.git build/web-tools/template
git clone --depth 1 --branch VER-2-10-4 https://github.com/freetype/freetype.git build/web-tools/freetype
git clone --depth 1 --branch 4.5.1 https://github.com/opencv/opencv.git build/web-tools/opencv
git -C build/web-tools/opencv apply "$PWD/scripts/opencv-emscripten.patch"
embuilder build ogg vorbis libpng
emcmake cmake -S build/web-tools/freetype -B build/web-tools/freetype-build \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DFT_WITH_PNG=ON \
    -DCMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=ON -DCMAKE_DISABLE_FIND_PACKAGE_BZip2=ON \
    -DCMAKE_DISABLE_FIND_PACKAGE_BrotliDec=ON
cmake --build build/web-tools/freetype-build -j4
emcmake cmake -S build/web-tools/opencv -B build/web-tools/opencv-build \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_LIST=core,imgproc \
    -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF -DBUILD_opencv_apps=OFF -DBUILD_JAVA=OFF -DBUILD_opencv_js=OFF \
    -DWITH_QUIRC=OFF -DWITH_JPEG=OFF -DWITH_PNG=OFF -DWITH_TIFF=OFF \
    -DWITH_WEBP=OFF -DWITH_OPENEXR=OFF -DWITH_OPENJPEG=OFF -DWITH_JASPER=OFF \
    -DWITH_PROTOBUF=OFF -DWITH_IPP=OFF -DWITH_TBB=OFF -DWITH_OPENCL=OFF \
    -DWITH_PTHREADS_PF=OFF -DWITH_ITT=OFF -DCV_ENABLE_INTRINSICS=OFF \
    -DCMAKE_CXX_FLAGS=-fexceptions \
    -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG -sUSE_PTHREADS=0" \
    -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG -sUSE_PTHREADS=0"
cmake --build build/web-tools/opencv-build --target opencv_imgproc -j4
emcmake cmake -S . -B build/web-build -DCMAKE_BUILD_TYPE=Release \
    -DSIV3D_WEB_TEMPLATE="$PWD/build/web-tools/template" \
    -DSIV3D_WEB_FREETYPE_BUILD="$PWD/build/web-tools/freetype-build" \
    -DSIV3D_WEB_OPENCV_BUILD="$PWD/build/web-tools/opencv-build"
cmake --build build/web-build --target siv3d_app -j4
```

`build/web-build/web/` のファイル一式を静的ホスティングに配置します。
ローカルで試す場合も `file://` ではなく HTTP を使用します。

```bash
build/web-tools/python/bin/python -m http.server 8000 --directory build/web-build/web
```

ブラウザーで `http://localhost:8000/` を開きます。3MFの保存先はブラウザーの
ダウンロード設定に従います。Web用辞書表は Linux 版から生成したもので、
全IDを比較する単体テストがあります。再生成する場合は OpenCV 4.x を使用して
`scripts/generate_web_dictionaries.cpp` と `src/aruco/marker_plate.cpp` を
ネイティブでコンパイルし、その出力を `src/aruco/web_dictionary_codes.inc` に保存します。
配布時は出力先の `licenses/` も一緒に配置してください。
Web版には FreeType Project のコードが含まれます。
Web版には日本語表示に必要なフォント、WebGL用シェーダー、描画用テクスチャだけを
同梱します。絵文字、アイコン、MIDI音源などを使う機能を追加する場合は、
`functional_test/CMakeLists.txt` のリソース一覧も更新してください。

GitHub Pages 用の `docs/` を更新するには、Web版をビルドした後に次を実行します。

```bash
cmake -E copy_directory build/web-build/web docs
```

`docs/` の変更をコミットし、GitHub Pages の公開元に設定したブランチへ反映します。
`docs/.nojekyll` はそのまま残してください。

## src に部品を追加する

実行しない関数やクラスは `src/` に追加します。例えば `src/hello.cpp` と
`src/hello.h` のように、実装とヘッダーファイルを配置します。
`.cpp` ファイルは `src/CMakeLists.txt` の `app_components` に1行追加します。
ライブラリのリンク設定や include 設定をファイルごとに書く必要はありません。

```cmake
add_library(app_components STATIC
    hello/hello.cpp
    camera/camera.cpp
)
```

実行するプログラムからはヘッダーファイルを `#include` して利用します。
`app_components` は、このアプリで複数の実行プログラムから再利用する部品をまとめる
ライブラリです。機能テストでは `add_default_build()` が自動的にリンクします。

`src/` のファイル自体を直接実行するのではなく、呼び出し側の実行ファイルを
ビルドして動作を確認します。

実行ファイルは `build/bin/` に生成されます。

## functional_test に実行用プログラムを追加する

実際に起動するプログラムは `functional_test/` のサブフォルダに追加します。
テストごとに `CMakeLists.txt` と実行用ソースファイルを作成し、
`functional_test/CMakeLists.txt` に `add_subdirectory()` を追加します。

```text
functional_test/hello/
├── CMakeLists.txt
└── hello_test.cpp
```

`functional_test/CMakeLists.txt`:

```cmake
add_subdirectory(hello)
```

`functional_test/hello/CMakeLists.txt`:

```cmake
add_default_build(hello_test)
```

`hello_test.cpp` の例:

```cpp
#include "hello.h"

#include <iostream>
#include <string>

int main()
{
    const std::string result = hello::hello();
    std::cout << result << '\\n';

    return 0;
}
```

実行ファイルは `build/bin/hello_test` に生成されます。
変更後は、セットアップのビルド手順を実行し、最後に
`ctest --test-dir build --output-on-failure` でテストを実行します。

OpenSiv3D に必要な Ubuntu パッケージは別途インストールしてください。
詳しくは OpenSiv3D 公式の Linux ビルド手順を確認してください。マーカー生成には
OpenCV 4.x の開発パッケージ (`libopencv-dev`, `aruco` コンポーネント) も必要です。

## GoogleTest の単体テストを追加する

単体テストは `test/` に置きます。GoogleTest は
`external/googletest/` の Git サブモジュールとして管理します。
初回取得時は次のコマンドを実行してください。

```bash
git submodule update --init --recursive
```

テストのソースファイルと同名のターゲットを `test/CMakeLists.txt` に追加します。

```text
test/
├── CMakeLists.txt
└── hello_UnitTest.cpp
```

`test/CMakeLists.txt`:

```cmake
include(${CMAKE_SOURCE_DIR}/cmake/add_gtest_executable.cmake)
add_gtest_executable(hello_UnitTest)
```

テストは次のコマンドでビルド・実行できます。

```bash
cmake --build build -j
ctest --test-dir build --output-on-failure
```
