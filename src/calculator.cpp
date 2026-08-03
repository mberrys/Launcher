#include "calculator.h"
#include "display.h"
#include "help_overlay.h"
#include "mykeyboard.h"
#include "powerSave.h"
#include <cmath>
#include <cstring>
#include <globals.h>

namespace {

constexpr int kMaxExpressionLength = 48;

bool isCalculatorChar(char ch) {
    return (ch >= '0' && ch <= '9') || ch == '.' || ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
           ch == '(' || ch == ')';
}

struct ExpressionParser {
    const char *cursor = nullptr;
    String error;

    void skipWhitespace() {
        while (*cursor == ' ') cursor++;
    }

    bool parseNumber(double &value) {
        skipWhitespace();
        const char *start = cursor;
        bool hasDigit = false;
        bool hasDot = false;

        while ((*cursor >= '0' && *cursor <= '9') || (*cursor == '.' && !hasDot)) {
            if (*cursor == '.') hasDot = true;
            else hasDigit = true;
            cursor++;
        }

        if (!hasDigit) {
            error = "Expected number";
            return false;
        }

        value = String(start).substring(0, static_cast<unsigned int>(cursor - start)).toDouble();
        return true;
    }

    bool parseFactor(double &value) {
        skipWhitespace();
        if (*cursor == '-') {
            cursor++;
            if (!parseFactor(value)) return false;
            value = -value;
            return true;
        }
        if (*cursor == '+') {
            cursor++;
            return parseFactor(value);
        }
        if (*cursor == '(') {
            cursor++;
            if (!parseExpression(value)) return false;
            skipWhitespace();
            if (*cursor != ')') {
                error = "Missing )";
                return false;
            }
            cursor++;
            return true;
        }
        return parseNumber(value);
    }

    bool parseTerm(double &value) {
        if (!parseFactor(value)) return false;
        skipWhitespace();
        while (*cursor == '*' || *cursor == '/') {
            const char op = *cursor++;
            double rhs = 0;
            if (!parseFactor(rhs)) return false;
            if (op == '*') {
                value *= rhs;
            } else {
                if (std::fabs(rhs) < 1e-12) {
                    error = "Divide by zero";
                    return false;
                }
                value /= rhs;
            }
            skipWhitespace();
        }
        return true;
    }

    bool parseExpression(double &value) {
        if (!parseTerm(value)) return false;
        skipWhitespace();
        while (*cursor == '+' || *cursor == '-') {
            const char op = *cursor++;
            double rhs = 0;
            if (!parseTerm(rhs)) return false;
            value = op == '+' ? value + rhs : value - rhs;
            skipWhitespace();
        }
        return true;
    }
};

bool evaluateExpression(const String &expression, double &result, String &error) {
    String trimmed = expression;
    trimmed.trim();
    if (trimmed.isEmpty()) {
        result = 0;
        error = "";
        return true;
    }

    ExpressionParser parser;
    parser.cursor = trimmed.c_str();
    if (!parser.parseExpression(result)) {
        error = parser.error.isEmpty() ? "Invalid expression" : parser.error;
        return false;
    }

    parser.skipWhitespace();
    if (*parser.cursor != '\0') {
        error = "Invalid expression";
        return false;
    }

    error = "";
    return true;
}

String formatResult(double value) {
    if (std::isnan(value) || std::isinf(value)) return "Error";

    const double rounded = std::round(value);
    if (std::fabs(value - rounded) < 1e-9) return String(static_cast<long long>(rounded));

    String out = String(value, 8);
    int dot = out.indexOf('.');
    if (dot >= 0) {
        while (out.endsWith("0")) out.remove(out.length() - 1);
        if (out.endsWith(".")) out.remove(out.length() - 1);
    }
    return out;
}

String tailForDisplay(const String &text, int maxChars) {
    if (static_cast<int>(text.length()) <= maxChars) return text;
    return String("...") + text.substring(text.length() - maxChars + 3);
}

void drawCalculatorUi(const String &expression, const String &resultLine, const String &error) {
    const int maxChars = tftWidth / (LW * FM);
    tft->fillScreen(BGCOLOR);
    tft->setTextColor(FGCOLOR, BGCOLOR);
    tft->setTextSize(FM);
    tft->drawString("Calculator", 6, 6);

    tft->setTextColor(FGCOLOR - 0x1111, BGCOLOR);
    tft->drawString("Expression", 6, 28);
    tft->setTextColor(FGCOLOR, BGCOLOR);
    tft->drawString(tailForDisplay(expression, maxChars), 6, 44);

    tft->setTextColor(FGCOLOR - 0x1111, BGCOLOR);
    tft->drawString("Result", 6, 68);
    tft->setTextColor(ALCOLOR, BGCOLOR);
    tft->drawString(tailForDisplay(resultLine, maxChars), 6, 84);

    if (!error.isEmpty()) {
        tft->setTextColor(ALCOLOR, BGCOLOR);
        tft->drawString(tailForDisplay(error, maxChars), 6, 108);
    }

    tft->setTextColor(FGCOLOR - 0x2222, BGCOLOR);
    tft->setTextSize(FP);
    tft->drawString("0-9  + - * /  ( )  .", 6, tftHeight - 36);
    tft->drawString("Enter = evaluate", 6, tftHeight - 24);
    tft->drawString("Del = backspace", 6, tftHeight - 12);
    tft->drawString("` = exit", tftWidth - (LW * FP * 8), tftHeight - 12);
}

} // namespace

void calculatorApp() {
#ifndef HAS_KEYBOARD
    displayError("Keyboard required");
    return;
#endif

    String expression;
    String resultLine = "0";
    String error;
    bool redraw = true;

    HelpScope help(kHelpCalculator);
    while (!returnToMenu) {
        if (helpOverlayCheck()) redraw = true;
        if (redraw) {
            drawCalculatorUi(expression, resultLine, error);
            redraw = false;
        }

        wakeUpScreen();
        checkPowerSaveTime();

        if (KeyStroke.pressed) {
            if (KeyStroke.exit_key) {
                resetGlobals();
                break;
            }

            if (KeyStroke.del) {
                if (!expression.isEmpty()) {
                    expression.remove(expression.length() - 1);
                    error = "";
                }
                redraw = true;
            } else if (KeyStroke.enter) {
                double value = 0;
                if (evaluateExpression(expression, value, error)) {
                    resultLine = formatResult(value);
                } else {
                    resultLine = "Error";
                }
                redraw = true;
            } else {
                for (char ch : KeyStroke.word) {
                    if (!isCalculatorChar(ch)) continue;
                    if (expression.length() >= kMaxExpressionLength) continue;
                    expression += ch;
                    error = "";
                }
                redraw = true;
            }

            resetGlobals();
        }

        if (check(EscPress)) break;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    tft->fillScreen(BGCOLOR);
}
