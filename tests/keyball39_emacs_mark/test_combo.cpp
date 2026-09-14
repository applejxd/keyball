#include <algorithm>
#include <iterator>
#include <vector>

#include "test_driver.hpp"
#include "test_fixture.hpp"
#include "test_keymap_key.hpp"

extern "C" {
#include "action_layer.h"
#include "quantum.h"
extern const uint16_t emacs_keymaps[][MATRIX_ROWS][MATRIX_COLS];
extern const uint8_t emacs_keymap_layer_count;
}

namespace {
constexpr unsigned click_term = 30;
constexpr unsigned settle_time = 100;

struct ClickChord {
    uint16_t first;
    uint16_t second;
    uint8_t button;
};

const ClickChord chords[] = {
    {KC_J, KC_K, 1},
    {KC_K, KC_L, 2},
    {KC_J, KC_L, 4},
    {KC_M, KC_COMM, 8},
    {KC_COMM, KC_DOT, 16},
};
}

class EmacsComboTiming : public TestFixture {
   protected:
    TestDriver driver;
    report_keyboard_t last_report = {};
    uint32_t keyboard_report_time = 0;
    uint32_t mouse_report_time = 0;
    std::vector<uint8_t> key_presses;
    std::vector<uint8_t> buttons;

    void SetUp() override {
        ON_CALL(driver, send_keyboard_mock(testing::_)).WillByDefault([this](report_keyboard_t& report) {
            for (uint8_t code : report.keys) {
                if (code && std::find(std::begin(last_report.keys), std::end(last_report.keys), code) == std::end(last_report.keys)) {
                    key_presses.push_back(code);
                }
            }
            last_report = report;
            keyboard_report_time = timer_read32();
        });
        ON_CALL(driver, send_mouse_mock(testing::_)).WillByDefault([this](report_mouse_t& report) {
            if (buttons.empty() ? report.buttons != 0 : buttons.back() != report.buttons) {
                buttons.push_back(report.buttons);
                mouse_report_time = timer_read32();
            }
        });
        EXPECT_CALL(driver, send_keyboard_mock(testing::_)).Times(testing::AnyNumber());
        EXPECT_CALL(driver, send_mouse_mock(testing::_)).Times(testing::AnyNumber());
        for (uint8_t layer = 0; layer < emacs_keymap_layer_count; ++layer) {
            for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
                for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
                    add_key(KeymapKey(layer, col, row, pgm_read_word(&emacs_keymaps[layer][row][col])));
                }
            }
        }
        scan(settle_time);
    }

    KeymapKey key(uint16_t code, uint8_t layer = 0) {
        auto found = std::find_if(keymap.begin(), keymap.end(), [=](const KeymapKey& candidate) {
            return candidate.layer == layer && candidate.code == code;
        });
        if (found == keymap.end()) {
            ADD_FAILURE() << "Missing production key " << code << " on layer " << +layer;
            return KeymapKey(0, 0, 0, KC_NO);
        }
        return *found;
    }

    void scan(unsigned ms = 1) {
        for (unsigned i = 0; i < ms; ++i) {
            run_one_scan_loop();
            housekeeping_task();
        }
    }

    void down(KeymapKey key, unsigned ms = 1) {
        key.press();
        scan(ms);
    }

    void up(KeymapKey key, unsigned ms = 1) {
        key.release();
        scan(ms);
    }

    void clear_reports() {
        key_presses.clear();
        buttons.clear();
    }

    void expect_released() {
        EXPECT_TRUE(std::all_of(std::begin(last_report.keys), std::end(last_report.keys), [](uint8_t code) { return code == 0; }));
        EXPECT_TRUE(buttons.empty() || buttons.back() == 0);
    }

    void TearDown() override {
        for (auto key : keymap) {
            if (matrix_is_on(key.position.row, key.position.col)) {
                key.release();
            }
        }
        scan(TAPPING_TERM * 2);
        expect_released();
        EXPECT_EQ(last_report.mods, 0);
    }
};

class EmacsCombo : public EmacsComboTiming, public testing::WithParamInterface<ClickChord> {};

TEST_P(EmacsCombo, ChordsThrough30MsKeepDraggingUntilLastRelease) {
    const auto chord = GetParam();
    for (unsigned gap : {1u, click_term - 1, click_term}) {
        for (bool reverse_press : {false, true}) {
            for (bool reverse_release : {false, true}) {
                SCOPED_TRACE(testing::Message() << "gap=" << gap << " reverse_press=" << reverse_press << " reverse_release=" << reverse_release);
                auto first = key(reverse_press ? chord.second : chord.first);
                auto second = key(reverse_press ? chord.first : chord.second);
                clear_reports();
                down(first, gap);
                down(second, settle_time);
                EXPECT_EQ(buttons, (std::vector<uint8_t>{chord.button}));
                scan(1000);
                EXPECT_EQ(buttons, (std::vector<uint8_t>{chord.button}));
                up(reverse_release ? second : first);
                EXPECT_EQ(buttons, (std::vector<uint8_t>{chord.button}));
                up(reverse_release ? first : second, settle_time);
                EXPECT_EQ(buttons, (std::vector<uint8_t>{chord.button, 0}));
                EXPECT_TRUE(key_presses.empty());
                expect_released();
            }
        }
    }
}

TEST_P(EmacsCombo, RollsBeyond30MsTypeBothKeysWithoutClicking) {
    const auto chord = GetParam();
    for (unsigned gap : {click_term + 1, 40u, 50u, 51u}) {
        for (bool reverse : {false, true}) {
            SCOPED_TRACE(testing::Message() << "gap=" << gap << " reverse=" << reverse);
            auto first = key(reverse ? chord.second : chord.first);
            auto second = key(reverse ? chord.first : chord.second);
            clear_reports();
            down(first, gap);
            down(second, settle_time);
            up(first);
            up(second, settle_time);
            EXPECT_TRUE(buttons.empty());
            EXPECT_EQ(key_presses, (std::vector<uint8_t>{static_cast<uint8_t>(first.code), static_cast<uint8_t>(second.code)}));
            expect_released();
        }
    }
}

TEST_P(EmacsCombo, NonOverlappingFastTapsRemainText) {
    const auto chord = GetParam();
    auto first = key(chord.first);
    auto second = key(chord.second);
    down(first);
    up(first);
    down(second);
    up(second, settle_time);
    EXPECT_TRUE(buttons.empty());
    EXPECT_EQ(key_presses, (std::vector<uint8_t>{static_cast<uint8_t>(chord.first), static_cast<uint8_t>(chord.second)}));
}

TEST_P(EmacsCombo, QuickClicksCanRepeatWithoutWaitingForTimeout) {
    const auto chord = GetParam();
    auto first = key(chord.first);
    auto second = key(chord.second);
    for (int repeat = 0; repeat < 3; ++repeat) {
        clear_reports();
        down(first);
        down(second);
        up(first);
        up(second);
        EXPECT_EQ(buttons, (std::vector<uint8_t>{chord.button, 0}));
        EXPECT_TRUE(key_presses.empty());
        expect_released();
    }
}

TEST_P(EmacsCombo, PhysicalModifiersRemainHeldAcrossClicks) {
    const auto chord = GetParam();
    for (uint16_t code : {KC_LCTL, KC_LALT, KC_LGUI, KC_RSFT}) {
        SCOPED_TRACE(code);
        auto modifier = key(code);
        clear_reports();
        down(modifier);
        down(key(chord.first));
        down(key(chord.second), settle_time);
        EXPECT_EQ(last_report.mods, MOD_BIT(code));
        EXPECT_EQ(buttons, (std::vector<uint8_t>{chord.button}));
        up(key(chord.first));
        up(key(chord.second), settle_time);
        EXPECT_EQ(last_report.mods, MOD_BIT(code));
        EXPECT_EQ(buttons, (std::vector<uint8_t>{chord.button, 0}));
        EXPECT_TRUE(key_presses.empty());
        up(modifier);
    }
}

TEST_P(EmacsCombo, HeldButtonsReleaseEvenAfterLayerTapChangesLayer) {
    const auto chord = GetParam();
    for (uint16_t code : {LT(1, KC_SPC), LT(2, KC_ENT), LT(3, KC_LNG1)}) {
        SCOPED_TRACE(code);
        auto layer = key(code);
        clear_reports();
        down(key(chord.first));
        down(key(chord.second), settle_time);
        down(layer, TAPPING_TERM + 1);
        EXPECT_TRUE(layer_state_is(QK_LAYER_TAP_GET_LAYER(code)));
        EXPECT_EQ(buttons, (std::vector<uint8_t>{chord.button}));
        up(key(chord.first));
        EXPECT_EQ(buttons, (std::vector<uint8_t>{chord.button}));
        up(key(chord.second));
        EXPECT_EQ(buttons, (std::vector<uint8_t>{chord.button, 0}));
        up(layer, settle_time);
        EXPECT_TRUE(key_presses.empty());
        expect_released();
    }
}

TEST_P(EmacsCombo, OtherLayersUseTheirOwnKeycodesInsteadOfBaseCombos) {
    const auto chord = GetParam();
    for (uint16_t code : {LT(1, KC_SPC), LT(2, KC_ENT), LT(3, KC_LNG1)}) {
        SCOPED_TRACE(code);
        auto layer = key(code);
        down(layer, TAPPING_TERM + 1);
        clear_reports();
        down(key(chord.first));
        down(key(chord.second), settle_time);
        up(key(chord.first));
        up(key(chord.second));
        up(layer, settle_time);
        EXPECT_TRUE(buttons.empty());
        EXPECT_FALSE(key_presses.empty());
        expect_released();
    }
}

TEST_P(EmacsCombo, CxOneShotKeepsExistingTransparentComboBehavior) {
    const auto chord = GetParam();
    auto layer = key(LT(3, KC_LNG1));
    auto cx = key(OSL(4), 3);
    down(layer, TAPPING_TERM + 1);
    down(cx);
    up(cx);
    up(layer, settle_time);
    ASSERT_TRUE(is_oneshot_layer_active());
    clear_reports();
    down(key(chord.first));
    down(key(chord.second), settle_time);
    up(key(chord.first));
    up(key(chord.second), settle_time);
    EXPECT_FALSE(is_oneshot_layer_active());
    if (chord.button >= 4) {
        EXPECT_EQ(buttons, (std::vector<uint8_t>{chord.button, 0}));
        EXPECT_TRUE(key_presses.empty());
    } else {
        EXPECT_TRUE(buttons.empty());
        EXPECT_FALSE(key_presses.empty());
    }
    expect_released();
}

TEST_P(EmacsCombo, CxOverHeldEmacsDoesNotRestoreBaseComboKeys) {
    const auto chord = GetParam();
    auto layer = key(LT(3, KC_LNG1));
    auto cx = key(OSL(4), 3);
    down(layer, TAPPING_TERM + 1);
    down(cx);
    up(cx);
    ASSERT_TRUE(is_oneshot_layer_active());
    clear_reports();
    down(key(chord.first));
    down(key(chord.second), settle_time);
    up(key(chord.first));
    up(key(chord.second));
    EXPECT_FALSE(is_oneshot_layer_active());
    EXPECT_TRUE(layer_state_is(3));
    EXPECT_TRUE(buttons.empty());
    EXPECT_FALSE(key_presses.empty());
    up(layer, settle_time);
    expect_released();
}

INSTANTIATE_TEST_CASE_P(AllButtons, EmacsCombo, testing::ValuesIn(chords));

TEST_F(EmacsComboTiming, FirstKeySetsTheClickDeadline) {
    uint32_t started = timer_read32();
    down(key(KC_J), 20);
    down(key(KC_K), click_term - 20 + 2);
    EXPECT_EQ(buttons, (std::vector<uint8_t>{1}));
    EXPECT_EQ(mouse_report_time - started, click_term + 1);
    EXPECT_TRUE(key_presses.empty());
    up(key(KC_J));
    up(key(KC_K));
    EXPECT_EQ(buttons, (std::vector<uint8_t>{1, 0}));
}

TEST_F(EmacsComboTiming, OtherComboKeysCannotExtendTheRollWindow) {
    down(key(KC_M), 20);
    down(key(KC_J), 20);
    down(key(KC_K), settle_time);
    up(key(KC_M));
    up(key(KC_J));
    up(key(KC_K), settle_time);
    EXPECT_TRUE(buttons.empty());
    EXPECT_EQ(key_presses, (std::vector<uint8_t>{KC_M, KC_J, KC_K}));
}

TEST_F(EmacsComboTiming, SingleComboKeyIsReportedAt31Ms) {
    uint32_t started = timer_read32();
    down(key(KC_J), click_term + 1);
    EXPECT_TRUE(key_presses.empty());
    scan(1);
    EXPECT_EQ(key_presses, (std::vector<uint8_t>{KC_J}));
    EXPECT_EQ(keyboard_report_time - started, click_term + 1);
    up(key(KC_J));
    EXPECT_TRUE(buttons.empty());
}
