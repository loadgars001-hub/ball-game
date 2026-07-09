#include "game.hpp"
#include "game_utils.hpp"
#include <algorithm>

void Game::openScriptEditor(ScriptTargetKind kind, int index) {
    scriptTargetKind  = kind;
    scriptTargetIndex = index;
    scriptEditError.clear();

    if (kind == ScriptTargetKind::Item && index >= 0 && index < (int)editLevel.items.size()) {
        scriptEditText = editLevel.items[index].scriptSource;
    } else if (kind == ScriptTargetKind::Wall && index >= 0 && index < (int)editLevel.walls.size()) {
        scriptEditText = editLevel.walls[index].scriptSource;
        if (scriptEditText.empty()) {
            scriptEditText =
                "-- Custom platform movement.\n"
                "local SPEED = 1.0\n"
                "local AMPLITUDE = 100\n\n"
                "function on_update(wall, dt)\n"
                "    wall.t = wall.t + dt\n"
                "    wall.x = wall.start_x + math.sin(wall.t * SPEED) * AMPLITUDE\n"
                "end\n";
        }
    }

    state = State::ScriptEdit;
}

void Game::applyScriptEditorChanges() {
    if (scriptTargetKind == ScriptTargetKind::Item) {
        if (scriptTargetIndex < 0 || scriptTargetIndex >= (int)editLevel.items.size()) return;
        Item& item = editLevel.items[scriptTargetIndex];
        item.loadScript(scriptEngine, scriptEditText, item.scriptName.empty() ? "item" : item.scriptName);
        if (!item.scriptOk) {
            scriptEditError = item.lastError;
        } else {
            scriptEditError.clear();
            state = State::Editor;
        }
    } else {
        if (scriptTargetIndex < 0 || scriptTargetIndex >= (int)editLevel.walls.size()) return;
        Wall& wall = editLevel.walls[scriptTargetIndex];
        wall.loadScript(scriptEngine, scriptEditText, "wall_script");
        if (!wall.scriptOk) {
            scriptEditError = wall.lastError;
        } else {
            scriptEditError.clear();
            state = State::Editor;
        }
    }
}

void Game::handleScriptEditorTextEntered(sf::Uint32 unicode) {
    bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
    if (ctrl) return;

    if (unicode == 8) { // Backspace
        if (!scriptEditText.empty()) scriptEditText.pop_back();
        return;
    }
    if (unicode == 13 || unicode == 10) { // Enter
        scriptEditText.push_back('\n');
        return;
    }
    if (unicode == 9) { // Tab -> 4 spaces
        scriptEditText += "    ";
        return;
    }
    if (unicode >= 32 && unicode < 127) { // printable ASCII только (Lua не нуждается в unicode)
        scriptEditText.push_back((char)unicode);
    }
}

void Game::pasteClipboardIntoScript() {
    std::string pasted = sf::Clipboard::getString().toAnsiString();
    // Нормализуем переносы строк: \r\n -> \n, одинокие \r -> \n
    std::string normalized;
    normalized.reserve(pasted.size());
    for (size_t i = 0; i < pasted.size(); ++i) {
        if (pasted[i] == '\r') {
            normalized.push_back('\n');
            if (i + 1 < pasted.size() && pasted[i + 1] == '\n') ++i;
        } else {
            normalized.push_back(pasted[i]);
        }
    }
    scriptEditText += normalized;
}

void Game::handleScriptEditorKey(sf::Keyboard::Key key, bool ctrl) {
    if (key == sf::Keyboard::Escape) {
        state = State::Editor;
        return;
    }
    if (ctrl && key == sf::Keyboard::Enter) {
        applyScriptEditorChanges();
        return;
    }
    if (ctrl && key == sf::Keyboard::V) {
        pasteClipboardIntoScript();
        return;
    }
    if (ctrl && key == sf::Keyboard::C) {
        sf::Clipboard::setString(scriptEditText);
        return;
    }
    if (ctrl && key == sf::Keyboard::X) {
        sf::Clipboard::setString(scriptEditText);
        scriptEditText.clear();
        return;
    }
    if (ctrl && key == sf::Keyboard::A) {
        return;
    }
}

void Game::renderScriptEditor() {
    window.clear(sf::Color(18, 19, 26));

    sf::Vector2f sz = {(float)window.getSize().x, (float)window.getSize().y};

    std::string title = (scriptTargetKind == ScriptTargetKind::Item)
        ? "Edit Item Script" : "Edit Wall Script";
    drawText(window, font, title, 30, {30.f, 20.f}, sf::Color(220, 220, 220));
    drawText(window, font, "Ctrl+Enter: Apply   Esc: Cancel   Ctrl+V: Paste   Ctrl+C: Copy", 18,
             {30.f, 56.f}, sf::Color(150, 150, 150));

    sf::FloatRect textArea(30.f, 96.f, sz.x - 60.f, sz.y - 220.f);
    sf::RectangleShape bg({textArea.width, textArea.height});
    bg.setPosition(textArea.left, textArea.top);
    bg.setFillColor(sf::Color(10, 11, 16));
    bg.setOutlineThickness(2.f);
    bg.setOutlineColor(sf::Color(80, 84, 100));
    window.draw(bg);

    if (fontOk) {
        sf::Text t(scriptEditText, font, 18);
        t.setFillColor(sf::Color(210, 230, 210));
        t.setPosition(textArea.left + 12.f, textArea.top + 10.f);
        window.draw(t);

        size_t lastNl = scriptEditText.find_last_of('\n');
        std::string lastLine = (lastNl == std::string::npos) ? scriptEditText : scriptEditText.substr(lastNl + 1);
        int lineCount = 1;
        for (char c : scriptEditText) if (c == '\n') lineCount++;

        sf::Text lastLineText(lastLine, font, 18);
        float lastLineWidth = lastLineText.getLocalBounds().width;
        float lineHeight = t.getCharacterSize() * 1.2f;

        if (((int)(goalPulse * 2.f)) % 2 == 0) {
            sf::RectangleShape cursor({2.f, 20.f});
            cursor.setPosition(textArea.left + 12.f + lastLineWidth + 2.f,
                                textArea.top + 10.f + (lineCount - 1) * lineHeight);
            cursor.setFillColor(sf::Color(120, 230, 120));
            window.draw(cursor);
        }
    } else {
        sf::RectangleShape warn({textArea.width - 24.f, 30.f});
        warn.setPosition(textArea.left + 12.f, textArea.top + 10.f);
        warn.setFillColor(sf::Color(120, 40, 40));
        window.draw(warn);
    }

    drawText(window, font, "Chars: " + std::to_string(scriptEditText.size()), 22,
             {sz.x - 200.f, 20.f}, sf::Color(255, 220, 60));

    if (!scriptEditError.empty()) {
        drawText(window, font, "Error: " + scriptEditError, 16,
                 {30.f, textArea.top + textArea.height + 10.f}, sf::Color(255, 110, 110));
    }

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mouse = {(float)mi.x, (float)mi.y};

    auto drawBtn = [&](const sf::FloatRect& r, const std::string& label, sf::Color base) {
        bool hover = pointInRect(mouse, r);
        sf::RectangleShape rect({r.width, r.height});
        rect.setPosition(r.left, r.top);
        rect.setFillColor(hover ? sf::Color(base.r+30, base.g+30, base.b+30) : base);
        rect.setOutlineThickness(2.f);
        rect.setOutlineColor(sf::Color(255,255,255,120));
        window.draw(rect);
        if (fontOk)
            drawText(window, font, label, 22,
                     {r.left + r.width/2.f, r.top + r.height/2.f - 14.f},
                     sf::Color::White, true);
    };
    drawBtn(btnScriptApplyRect,  "Apply (Ctrl+Enter)", sf::Color(30, 120, 60));
    drawBtn(btnScriptCancelRect, "Cancel (Esc)",       sf::Color(120, 40, 40));
    drawBtn(btnScriptPasteRect,  "Paste (Ctrl+V)",     sf::Color(40, 80, 130));

    window.display();
}
