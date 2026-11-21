#include <Siv3D.hpp>

#include "marker_plate.h"
#include "numeric_input.h"

#include <array>
#include <cmath>
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

std::string defaultFileName(
        const markerplate::DictionaryInfo& dictionary, int id, double markerSizeMm, double additionalMarginMm) {
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

    std::string fileName = dictionaryName + "_id" + std::to_string(id) + "_" + sizeText + "mm";
    if (additionalMarginMm > 0.0) {
        std::ostringstream additionalMargin;
        additionalMargin.imbue(std::locale::classic());
        additionalMargin << std::fixed << std::setprecision(2) << additionalMarginMm;
        std::string additionalMarginText = additionalMargin.str();
        while (!additionalMarginText.empty() && additionalMarginText.back() == '0') {
            additionalMarginText.pop_back();
        }
        if (!additionalMarginText.empty() && additionalMarginText.back() == '.') {
            additionalMarginText.pop_back();
        }
        fileName += "_extra" + additionalMarginText + "mm";
    }
    return fileName + ".3mf";
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
    const Font previewLegendFont{14};
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
    TextEditState additionalMarginText{U"0"};
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

        const std::array<std::pair<String, TextEditState*>, 5> fields{
                {{U"マーカーID (0-{})"_fmt(dictionary.markerCount - 1), &idText},
                 {U"サイズ (mm)", &sizeText},
                 {U"白い土台の厚さ (mm)", &baseText},
                 {U"黒の高さ (mm)", &reliefText},
                 {U"各辺の追加余白 (mm)", &additionalMarginText}}};
        int y = 286;
        bool textChanged = false;
        bool rejectedInput = false;
        for (const auto& [label, text] : fields) {
            labelFont(label).draw(30, y + 6, Palette::Black);
            SimpleGUI::TextBox(*text, Vec2{270, static_cast<double>(y)}, 140, 24);
            textChanged |= text->textChanged;
            rejectedInput |= filterNumericInput(text->text.str(), text->cursorPos, text != &idText);
            y += 48;
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
        const auto parsedAdditionalMargin = ParseFloatOpt<double>(additionalMarginText.text);
        const bool validInput = parsedId && parsedSize && parsedBase && parsedRelief && parsedAdditionalMargin;
        const bool validAdditionalMargin =
                parsedAdditionalMargin && std::isfinite(*parsedAdditionalMargin) && *parsedAdditionalMargin >= 0.0 &&
                *parsedAdditionalMargin <= 1000.0;
        const bool validId =
                validInput && validAdditionalMargin && (*parsedId >= 0) && (*parsedId < dictionary.markerCount);

        labelFont(U"サイズ指定の基準").draw(30, 520, ColorF{0.2});
        SimpleGUI::RadioButtons(sizeReference, sizeOptions, Vec2{30, 548}, 380);
        const double additionalMargin = parsedAdditionalMargin.value_or(0.0);
        const double markerSize =
                parsedSize ? (sizeReference == 0 ? *parsedSize
                                                 : markerplate::markerSizeForPlateSize(
                                                           dictionary.type, *parsedSize, additionalMargin))
                           : 0.0;
        const double boardSize =
                markerplate::plateSizeForMarkerSize(dictionary.type, markerSize, additionalMargin);
        const bool validPreviewDimensions =
                std::isfinite(markerSize) && markerSize > 0.0 && std::isfinite(boardSize) && boardSize > 0.0;
        const double previewScale = validPreviewDimensions ? 280.0 / boardSize : 0.0;
        const double cell = validPreviewDimensions ? markerSize / (dictionary.payloadSize + 2.0) * previewScale : 0.0;
        const double markerOffset =
                validPreviewDimensions
                        ? (markerSize / (dictionary.payloadSize + 2.0) + additionalMargin) * previewScale
                        : 0.0;
        RectF{470, 100, 360, 470}.rounded(12).draw(ColorF{0.82, 0.85, 0.88});
        const ColorF extraMarginColor{0.85, 0.92, 1.0};
        const ColorF plateColor = additionalMargin > 0.0 ? extraMarginColor : ColorF{1.0};
        RectF{510, 120, 280, 280}.draw(plateColor);
        const double extraMarginPixels = additionalMargin * previewScale;
        if (validPreviewDimensions && additionalMargin > 0.0) {
            const double originalPlateOffset = extraMarginPixels;
            const double originalPlateSize = 280.0 - 2.0 * originalPlateOffset;
            RectF{510 + originalPlateOffset, 120 + originalPlateOffset, originalPlateSize, originalPlateSize}
                    .draw(Palette::White);
            const ColorF marginBoundaryColor{0.25, 0.5, 0.75};
            const double right = 510 + originalPlateOffset + originalPlateSize;
            const double bottom = 120 + originalPlateOffset + originalPlateSize;
            Line{510 + originalPlateOffset, 120 + originalPlateOffset, right, 120 + originalPlateOffset}
                    .draw(1, marginBoundaryColor);
            Line{right, 120 + originalPlateOffset, right, bottom}.draw(1, marginBoundaryColor);
            Line{right, bottom, 510 + originalPlateOffset, bottom}.draw(1, marginBoundaryColor);
            Line{510 + originalPlateOffset, bottom, 510 + originalPlateOffset, 120 + originalPlateOffset}
                    .draw(1, marginBoundaryColor);
        }
        const int markerGridSize = dictionary.payloadSize + 2;
        if (validId && validPreviewDimensions) {
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
                        RectF{510 + markerOffset + column * cell, 120 + markerOffset + row * cell, cell, cell}
                                .draw(Palette::Black);
                    }
                }
            }
        }
        const ColorF dimensionColor{0.2};
        if (validPreviewDimensions) {
            Line{510 + markerOffset, 400 - markerOffset, 510 + markerOffset, 420}.draw(1, dimensionColor);
            Line{790 - markerOffset, 400 - markerOffset, 790 - markerOffset, 420}.draw(1, dimensionColor);
            Line{510 + markerOffset, 420, 790 - markerOffset, 420}.drawDoubleHeadedArrow(2, SizeF{8, 8}, dimensionColor);
        }
        Line{510, 400, 510, 480}.draw(1, dimensionColor);
        Line{790, 400, 790, 480}.draw(1, dimensionColor);
        Line{510, 480, 790, 480}.drawDoubleHeadedArrow(2, SizeF{8, 8}, dimensionColor);
        if (parsedSize && validPreviewDimensions) {
            labelFont(U"マーカー幅: {:.2f} mm"_fmt(markerSize)).drawAt(650, 445, dimensionColor);
            labelFont(U"余白を含む幅: {:.2f} mm"_fmt(boardSize)).drawAt(650, 505, dimensionColor);
        }
        previewLegendFont(U"青: 追加余白　青枠〜黒枠: 1セル").drawAt(650, 540, ColorF{0.2});

        if (SimpleGUI::Button(U"3MFを書き出す", Vec2{30, 650}, 380, validId)) {
            inputWarning.clear();
            try {
                const markerplate::Parameters parameters{
                        *parsedId, markerSize, *parsedBase, *parsedRelief, dictionary.type, additionalMargin};
#ifdef __EMSCRIPTEN__
                const auto bytes = markerplate::make3mf(parameters);
                const auto filename = defaultFileName(dictionary, *parsedId, markerSize, additionalMargin);
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
                        Unicode::Widen(defaultFileName(dictionary, *parsedId, markerSize, additionalMargin)));
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
        } else if (not validAdditionalMargin) {
            displayStatus = U"追加余白は0〜1000 mmで入力してください。";
            displayIsError = true;
        } else if (not validId) {
            displayStatus = U"選択中の辞書のID範囲: 0〜{}"_fmt(dictionary.markerCount - 1);
            displayIsError = true;
        }
        labelFont(displayStatus)
                .draw(30, 720, displayIsError ? ColorF{1.0, 0.0, 0.0} : ColorF{0.1, 0.45, 0.2});
    }
}
