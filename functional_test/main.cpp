#include <Siv3D.hpp>

#include "marker_plate.h"
#include "numeric_input.h"

#include <array>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <utility>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(int, download3mf, (const std::uint8_t* data, std::size_t size, const char* filename), {
    try {
        const blob = new Blob([HEAPU8.slice(data, data + size)], { type: "model/3mf" });
        const url = URL.createObjectURL(blob);
        const link = document.createElement("a");
        link.href = url;
        link.download = UTF8ToString(filename);
        link.click();
        setTimeout(() => URL.revokeObjectURL(url), 60000);
        return 1;
    } catch (error) {
        console.error("3MF download failed:", error);
        return 0;
    }
});
#endif

std::string defaultFileName(const markerplate::DictionaryInfo& dictionary, int id, double markerSizeMm) {
    std::string dictionaryName;
    for (const unsigned char character : dictionary.name) {
        const bool isAsciiAlphanumeric =
                (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
                (character >= '0' && character <= '9');
        if (isAsciiAlphanumeric) {
            dictionaryName.push_back(static_cast<char>(character));
        } else if (!dictionaryName.empty() && dictionaryName.back() != '_') {
            dictionaryName.push_back('_');
        }
    }
    while (!dictionaryName.empty() && dictionaryName.back() == '_') {
        dictionaryName.pop_back();
    }

    std::ostringstream size;
    size.imbue(std::locale::classic());
    size << std::fixed << std::setprecision(2) << markerSizeMm;
    std::string sizeText = size.str();
    while (!sizeText.empty() && sizeText.back() == '0') {
        sizeText.pop_back();
    }
    if (!sizeText.empty() && sizeText.back() == '.') {
        sizeText.pop_back();
    }

    return dictionaryName + "_id" + std::to_string(id) + "_" + sizeText + "mm.3mf";
}

void Main() {
    Window::SetTitle(U"MarkerPlateMaker");
    Window::Resize(900, 770);
#ifdef __EMSCRIPTEN__
    Scene::SetResizeMode(ResizeMode::Keep);
    Scene::Resize(900, 770);
#endif

    const Font titleFont{28};
    const Font labelFont{18};
    const auto dictionaries = markerplate::supportedDictionaries();
    Array<String> dictionaryNames;
    for (const auto& dictionary : dictionaries) {
        dictionaryNames << Unicode::Widen(std::string{dictionary.name});
    }
    ListBoxState dictionarySelection{dictionaryNames};
    dictionarySelection.selectedItemIndex = size_t{0};
    TextEditState idText{U"0"};
    TextEditState sizeText{U"50"};
    TextEditState baseText{U"2"};
    TextEditState reliefText{U"0.4"};
    const Array<String> sizeOptions{U"マーカー自体", U"白い余白を含む"};
    size_t sizeReference = 0;
    String status;
    String inputWarning;
    bool statusIsError = false;

    while (System::Update()) {
        Scene::SetBackground(ColorF{0.94, 0.95, 0.97});

        labelFont(U"辞書").draw(30, 55, Palette::Black);
        SimpleGUI::ListBox(dictionarySelection, Vec2{30, 80}, 380, 195);
        const auto dictionaryIndex = dictionarySelection.selectedItemIndex.value_or(0);
        const auto& dictionary = dictionaries[dictionaryIndex];

        const std::array<std::pair<String, TextEditState*>, 4> fields{
                {{U"マーカーID (0-{})"_fmt(dictionary.markerCount - 1), &idText},
                 {U"サイズ (mm)", &sizeText},
                 {U"白い土台の厚さ (mm)", &baseText},
                 {U"黒の高さ (mm)", &reliefText}}};
        int y = 300;
        bool textChanged = false;
        bool rejectedInput = false;
        for (const auto& [label, text] : fields) {
            labelFont(label).draw(30, y + 6, Palette::Black);
            SimpleGUI::TextBox(*text, Vec2{270, static_cast<double>(y)}, 140, 24);
            textChanged |= text->textChanged;
            rejectedInput |= filterNumericInput(text->text.str(), text->cursorPos, text != &idText);
            y += 58;
        }
        if (rejectedInput) {
            inputWarning = U"IDは半角数字、寸法は半角数字とドットで入力してください。";
        } else if (textChanged) {
            inputWarning.clear();
        }

        if (TextInput::GetEditingText().isEmpty()) {
            for (size_t i = 0; i < fields.size(); ++i) {
                if (fields[i].second->tabKey ||
                    (fields[i].second->active && KeyShift.pressed() && KeyTab.down())) {
                    fields[i].second->active = false;
                    const size_t next =
                            KeyShift.pressed() ? (i + fields.size() - 1) % fields.size() : (i + 1) % fields.size();
                    auto& target = *fields[next].second;
                    target.active = true;
                    target.cursorPos = target.text.size();
#ifdef __EMSCRIPTEN__
                    target.leftPressStopwatch.reset();
                    target.rightPressStopwatch.reset();
                    target.cursorStopwatch.restart();
#else
                    target.resetStopwatches();
#endif
                    break;
                }
            }
        }

        const auto parsedId = ParseIntOpt<int32>(idText.text);
        const auto parsedSize = ParseFloatOpt<double>(sizeText.text);
        const auto parsedBase = ParseFloatOpt<double>(baseText.text);
        const auto parsedRelief = ParseFloatOpt<double>(reliefText.text);
        const bool validInput = parsedId && parsedSize && parsedBase && parsedRelief;
        const bool validId = validInput && (*parsedId >= 0) && (*parsedId < dictionary.markerCount);

        labelFont(U"サイズ指定の基準").draw(30, 530, ColorF{0.2});
        SimpleGUI::RadioButtons(sizeReference, sizeOptions, Vec2{30, 560}, 380);
        const double markerSize =
                parsedSize ? (sizeReference == 0 ? *parsedSize
                                                 : markerplate::markerSizeForPlateSize(dictionary.type, *parsedSize))
                           : 0.0;
        const double boardSize = markerplate::plateSizeForMarkerSize(dictionary.type, markerSize);
        RectF{470, 100, 360, 440}.rounded(12).draw(ColorF{0.82, 0.85, 0.88});
        RectF{510, 120, 280, 280}.draw(Palette::White);
        const int markerGridSize = dictionary.payloadSize + 2;
        const double cell = 280.0 / (markerGridSize + 2);
        if (validId) {
            const auto blackCells = markerplate::blackCellsForId(dictionary.type, *parsedId);
            for (int row = 0; row < markerGridSize; ++row) {
                for (int column = 0; column < markerGridSize; ++column) {
                    bool isBlack = (row == 0) || (row == markerGridSize - 1) || (column == 0) ||
                                   (column == markerGridSize - 1);
                    if (!isBlack) {
                        const auto bitIndex =
                                static_cast<std::size_t>((row - 1) * dictionary.payloadSize + (column - 1));
                        isBlack = (blackCells[bitIndex] != 0);
                    }
                    if (isBlack) {
                        RectF{510 + (column + 1) * cell, 120 + (row + 1) * cell, cell, cell}.draw(Palette::Black);
                    }
                }
            }
        }
        const ColorF dimensionColor{0.2};
        Line{510 + cell, 400 - cell, 510 + cell, 420}.draw(1, dimensionColor);
        Line{790 - cell, 400 - cell, 790 - cell, 420}.draw(1, dimensionColor);
        Line{510 + cell, 420, 790 - cell, 420}.drawDoubleHeadedArrow(2, SizeF{8, 8}, dimensionColor);
        Line{510, 400, 510, 480}.draw(1, dimensionColor);
        Line{790, 400, 790, 480}.draw(1, dimensionColor);
        Line{510, 480, 790, 480}.drawDoubleHeadedArrow(2, SizeF{8, 8}, dimensionColor);
        if (parsedSize) {
            labelFont(U"マーカー幅: {:.2f} mm"_fmt(markerSize)).drawAt(650, 445, dimensionColor);
            labelFont(U"余白を含む幅: {:.2f} mm"_fmt(boardSize)).drawAt(650, 505, dimensionColor);
        }

        if (SimpleGUI::Button(U"3MFを書き出す", Vec2{30, 650}, 380, validId)) {
            inputWarning.clear();
            try {
                const markerplate::Parameters parameters{
                        *parsedId, markerSize, *parsedBase, *parsedRelief, dictionary.type};
#ifdef __EMSCRIPTEN__
                const auto bytes = markerplate::make3mf(parameters);
                const auto filename = defaultFileName(dictionary, *parsedId, markerSize);
                if (!download3mf(bytes.data(), bytes.size(), filename.c_str())) {
                    status = U"3MFのダウンロードを開始できませんでした。";
                    statusIsError = true;
                } else {
                    status = U"3MFのダウンロードを開始しました。";
                    statusIsError = false;
                }
#else
                const auto outputPath = Dialog::SaveFile(
                        {FileFilter{U"3D Manufacturing Format", {U"3mf"}}},
                        U"",
                        U"マーカーを3MFで保存",
                        Unicode::Widen(defaultFileName(dictionary, *parsedId, markerSize)));
                if (outputPath) {
                    auto filePath = std::filesystem::path{Unicode::ToUTF8(*outputPath)};
                    filePath.replace_extension(".3mf");
                    markerplate::save3mf(filePath, parameters);
                    status = U"3MFで保存しました: " + Unicode::Widen(filePath.string());
                    statusIsError = false;
                }
#endif
            } catch (const std::exception& error) {
                status = Unicode::Widen(error.what());
                statusIsError = true;
            }
        }

        String displayStatus = status;
        bool displayIsError = statusIsError;
        if (inputWarning) {
            displayStatus = inputWarning;
            displayIsError = true;
        } else if (not validInput) {
            displayStatus = U"入力値を確認してください。";
            displayIsError = true;
        } else if (not validId) {
            displayStatus = U"選択中の辞書のID範囲: 0〜{}"_fmt(dictionary.markerCount - 1);
            displayIsError = true;
        }
        labelFont(displayStatus)
                .draw(30, 720, displayIsError ? ColorF{1.0, 0.0, 0.0} : ColorF{0.1, 0.45, 0.2});
    }
}
