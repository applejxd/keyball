#include <algorithm>
#include <iterator>
#include <vector>

#include "test_driver.hpp"
#include "test_fixture.hpp"
#include "test_keymap_key.hpp"

extern "C" {
#include "action_layer.h"
#include "quantum.h"
#include "lib/keyball/keyball.h"
extern bool set_mark_active;
extern const uint16_t emacs_keymaps[][MATRIX_ROWS][MATRIX_COLS];
extern const uint8_t emacs_keymap_layer_count;
}

using testing::_;
using testing::AnyNumber;

namespace {
constexpr uint16_t cut_line = KEYBALL_SAFE_RANGE;
constexpr uint16_t set_mark = KEYBALL_SAFE_RANGE + 1;
constexpr uint16_t abort_mark = KEYBALL_SAFE_RANGE + 2;
constexpr uint16_t copy_region = KEYBALL_SAFE_RANGE + 3;
constexpr uint16_t cut_word = KEYBALL_SAFE_RANGE + 4;
constexpr uint16_t cut_word_backward = KEYBALL_SAFE_RANGE + 5;
constexpr uint16_t mark_word = KEYBALL_SAFE_RANGE + 6;
}

class EmacsMark : public TestFixture {
   protected:
    struct TimedReport {
        report_keyboard_t report;
        uint32_t time;
    };

    TestDriver driver;
    report_keyboard_t last_report = {};
    std::vector<report_keyboard_t> reports;
    std::vector<TimedReport> timed_reports;
    std::vector<report_mouse_t> mouse_reports;

    KeymapKey mark{0, 0, 0, set_mark};
    KeymapKey abort{0, 1, 0, abort_mark};
    KeymapKey left{0, 2, 0, KC_LEFT};
    KeymapKey right{0, 3, 0, KC_RIGHT};
    KeymapKey lshift{0, 4, 0, KC_LSFT};
    KeymapKey rshift{0, 5, 0, KC_RSFT};

    void SetUp() override {
        ON_CALL(driver, send_keyboard_mock(_)).WillByDefault([this](report_keyboard_t& report) {
            last_report = report;
            reports.push_back(report);
            timed_reports.push_back({report, timer_read32()});
        });
        ON_CALL(driver, send_mouse_mock(_)).WillByDefault([this](report_mouse_t& report) {
            mouse_reports.push_back(report);
        });
        EXPECT_ANY_REPORT(driver).Times(AnyNumber());
        EXPECT_CALL(driver, send_mouse_mock(_)).Times(AnyNumber());
        set_keymap({mark, abort, left, right, lshift, rshift});
        set_mark_active = false;
    }

    void scan(unsigned ms = 1) {
        for (unsigned i = 0; i < ms; ++i) {
            run_one_scan_loop();
            // The production main loop calls this after keyboard_task().
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

    void tap(KeymapKey key, unsigned ms = 1) {
        down(key, ms);
        up(key);
    }

    void expect_shift(bool pressed) {
        EXPECT_EQ((last_report.mods & MOD_MASK_SHIFT) != 0, pressed);
    }

    KeymapKey extra(uint16_t code, uint8_t col = 6, uint8_t row = 0) {
        KeymapKey key(0, col, row, code);
        add_key(key);
        return key;
    }

    void overlay(uint8_t layer, std::initializer_list<KeymapKey> overrides = {}) {
        for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
            for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
                auto found = std::find_if(overrides.begin(), overrides.end(), [=](const KeymapKey& key) {
                    return key.position.row == row && key.position.col == col;
                });
                add_key(found == overrides.end() ? KeymapKey(layer, col, row, KC_TRNS) : *found);
            }
        }
    }

    void expect_key_reports(uint8_t code, uint8_t required_mods, uint8_t forbidden_mods = 0) {
        bool found = false;
        for (const auto& report : reports) {
            if (std::count(std::begin(report.keys), std::end(report.keys), code)) {
                found = true;
                EXPECT_EQ(report.mods & required_mods, required_mods);
                EXPECT_EQ(report.mods & forbidden_mods, 0);
            }
        }
        EXPECT_TRUE(found);
    }

    void expect_exact_key_reports(uint8_t code, uint8_t mods) {
        expect_key_reports(code, mods, static_cast<uint8_t>(~mods));
    }

    void expect_no_key_reports(uint8_t code) {
        for (const auto& report : reports) {
            EXPECT_EQ(std::count(std::begin(report.keys), std::end(report.keys), code), 0);
        }
    }

    bool has_key(uint8_t keycode) const {
        return std::find(std::begin(last_report.keys), std::end(last_report.keys), keycode) != std::end(last_report.keys);
    }

    void TearDown() override {
        if (set_mark_active) {
            tap(abort);
        }
        for (auto key : keymap) {
            if (matrix_is_on(key.position.row, key.position.col)) {
                key.release();
            }
        }
        scan(TAPPING_TERM * 2);
        EXPECT_EQ(last_report.mods, 0);
    }
};

TEST_F(EmacsMark, ProductionKeymapReservesWeakRightShiftForMark) {
    auto uses_weak_right_shift = [](uint16_t code) {
        uint8_t basic = QK_MODS_GET_BASIC_KEYCODE(code);
        return IS_QK_MODS(code) && (code & QK_RSFT) == QK_RSFT && basic != KC_NO && !IS_MODIFIER_KEYCODE(basic);
    };
    EXPECT_TRUE(uses_weak_right_shift(RSFT(KC_1)));
    EXPECT_FALSE(uses_weak_right_shift(S(KC_1)));
    EXPECT_FALSE(uses_weak_right_shift(RSFT(KC_RSFT)));

    for (uint8_t layer = 0; layer < emacs_keymap_layer_count; ++layer) {
        for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
            for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
                uint16_t code = pgm_read_word(&emacs_keymaps[layer][row][col]);
                EXPECT_FALSE(uses_weak_right_shift(code)) << "layer " << +layer << ", row " << +row << ", col " << +col;
            }
        }
    }
}

TEST_F(EmacsMark, PhysicalLeftShiftSurvivesNavigationRelease) {
    down(lshift);
    tap(mark);
    down(left);
    up(left);
    EXPECT_EQ(last_report.mods & MOD_BIT(KC_LSFT), MOD_BIT(KC_LSFT));
    up(lshift);
}

TEST_F(EmacsMark, OverlappingNavigationKeepsShiftUntilLastRelease) {
    tap(mark);
    down(left);
    down(right);
    up(left);
    EXPECT_TRUE(has_key(KC_RIGHT));
    expect_shift(true);
    up(right);
    expect_shift(false);
}

TEST_F(EmacsMark, AbortWhileNavigationHeldImmediatelyReleasesAuxiliaryShift) {
    tap(mark);
    down(left);
    tap(abort);
    EXPECT_FALSE(set_mark_active);
    EXPECT_TRUE(has_key(KC_LEFT));
    expect_shift(false);
    for (const auto& report : reports) {
        EXPECT_EQ(std::count(std::begin(report.keys), std::end(report.keys), KC_ESC), 0);
    }
    up(left);
    expect_shift(false);
}

TEST_F(EmacsMark, MarkAloneDoesNotShiftTypingOrClicking) {
    auto a = extra(KC_A);
    auto j = extra(KC_J, 7);
    auto k = extra(KC_K, 8);
    tap(mark);
    EXPECT_TRUE(set_mark_active);
    expect_shift(false);
    tap(a);
    expect_key_reports(KC_A, 0, MOD_MASK_SHIFT);
    down(j);
    down(k, COMBO_TERM + 2);
    ASSERT_FALSE(mouse_reports.empty());
    EXPECT_EQ(mouse_reports.back().buttons, 1);
    expect_shift(false);
    up(j);
    EXPECT_EQ(mouse_reports.back().buttons, 1);
    up(k);
    EXPECT_EQ(mouse_reports.back().buttons, 0);
}

TEST_F(EmacsMark, AllBareNavigationKeysCanRepeatAndRemainHeld) {
    tap(mark);
    for (uint16_t code : {KC_LEFT, KC_RIGHT, KC_UP, KC_DOWN, KC_HOME, KC_END, KC_PGUP, KC_PGDN}) {
        SCOPED_TRACE(code);
        KeymapKey nav(0, 6, 0, code);
        set_keymap({mark, abort, left, right, lshift, rshift, nav});
        for (int repetition = 0; repetition < 3; ++repetition) {
            down(nav, TAPPING_TERM + 1);
            EXPECT_TRUE(has_key(code));
            expect_shift(true);
            up(nav);
            expect_shift(false);
        }
    }
}

TEST_F(EmacsMark, PhysicalShiftsSurviveMarkEndingWhileNavigationHeld) {
    down(lshift);
    down(rshift);
    tap(mark);
    down(left);
    tap(abort);
    EXPECT_EQ(last_report.mods & MOD_MASK_SHIFT, MOD_MASK_SHIFT);
    up(left);
    EXPECT_EQ(last_report.mods & MOD_MASK_SHIFT, MOD_MASK_SHIFT);
    up(lshift);
    EXPECT_EQ(last_report.mods & MOD_MASK_SHIFT, MOD_BIT(KC_RSFT));
    up(rshift);
}

TEST_F(EmacsMark, ModTapShiftsSurviveMarkEnding) {
    for (uint16_t code : {LSFT_T(KC_Z), RSFT_T(KC_SLSH)}) {
        SCOPED_TRACE(code);
        KeymapKey modtap(0, 6, 0, code);
        set_keymap({mark, abort, left, right, lshift, rshift, modtap});
        down(modtap, TAPPING_TERM + 1);
        uint8_t physical_mods = get_mods();
        EXPECT_NE(physical_mods & MOD_MASK_SHIFT, 0);
        tap(mark);
        down(left);
        tap(abort);
        up(left);
        EXPECT_EQ(get_mods(), physical_mods);
        EXPECT_EQ(last_report.mods & MOD_MASK_SHIFT, physical_mods & MOD_MASK_SHIFT);
        up(modtap);
        expect_shift(false);
        scan(TAPPING_TERM + 1);
    }
}

TEST_F(EmacsMark, EveryExistingEndingKeyClearsMarkBeforeItsReport) {
    for (uint16_t code : std::initializer_list<uint16_t>{C(KC_C), C(KC_X), C(KC_V), RCTL(KC_C), RCTL(KC_X), RCTL(KC_V), KC_DEL, set_mark}) {
        SCOPED_TRACE(code);
        KeymapKey end(0, 6, 0, code);
        set_keymap({mark, abort, left, right, lshift, rshift, end});
        tap(mark);
        down(left);
        reports.clear();
        down(end);
        EXPECT_FALSE(set_mark_active);
        expect_shift(false);
        for (const auto& report : reports) {
            EXPECT_EQ(report.mods & MOD_MASK_SHIFT, 0);
        }
        up(end);
        up(left);
        expect_shift(false);
    }
}

TEST_F(EmacsMark, CopyEndingPreservesPhysicalControlAndShift) {
    auto control = extra(KC_LCTL);
    auto copy = extra(KC_C, 7);
    down(control);
    down(lshift);
    tap(mark);
    down(left);
    reports.clear();
    tap(copy);
    EXPECT_FALSE(set_mark_active);
    expect_key_reports(KC_C, MOD_BIT(KC_LCTL) | MOD_BIT(KC_LSFT), MOD_BIT(KC_RSFT));
    EXPECT_EQ(get_mods(), MOD_BIT(KC_LCTL) | MOD_BIT(KC_LSFT));
    up(left);
    up(control);
    up(lshift);
}

TEST_F(EmacsMark, AbortWhenOffSendsEscape) {
    tap(abort);
    expect_key_reports(KC_ESC, 0, MOD_MASK_SHIFT);
    EXPECT_FALSE(set_mark_active);
}

TEST_F(EmacsMark, NavigationHeldBeforeMarkDoesNotAcquireOrReleaseItsShift) {
    down(left);
    tap(mark);
    expect_shift(false);
    down(right);
    expect_shift(true);
    up(left);
    expect_shift(true);
    up(right);
    expect_shift(false);
}

TEST_F(EmacsMark, LateReleaseFromPreviousMarkSessionDoesNotClearNewNavigation) {
    tap(mark);
    down(left);
    tap(abort);
    tap(mark);
    expect_shift(false);
    down(right);
    up(left);
    EXPECT_TRUE(has_key(KC_RIGHT));
    expect_shift(true);
    up(right);
    expect_shift(false);
}

TEST_F(EmacsMark, DuplicateNavigationKeycodesTrackPhysicalKeysSeparately) {
    auto another_left = extra(KC_LEFT);
    tap(mark);
    down(left);
    down(another_left);
    up(left);
    expect_shift(true);
    up(another_left);
    expect_shift(false);
}

TEST_F(EmacsMark, NewKeypressAndComboBufferFlushDoNotLoseHeldNavigationShift) {
    auto a = extra(KC_A);
    auto j = extra(KC_J, 7);
    tap(mark);
    down(left);
    reports.clear();
    tap(a);
    expect_key_reports(KC_A, 0);
    for (const auto& report : reports) {
        EXPECT_NE(report.mods & MOD_MASK_SHIFT, 0);
    }
    down(j, COMBO_TERM + 2);
    expect_shift(true);
    scan(TAPPING_TERM + 1);
    reports.clear();
    tap(a);
    for (const auto& report : reports) {
        EXPECT_NE(report.mods & MOD_MASK_SHIFT, 0);
    }
    up(j);
    up(left);
    expect_shift(false);
}

TEST_F(EmacsMark, ShiftedSymbolReleaseDoesNotStealMarkShift) {
    auto symbol = extra(S(KC_1));
    tap(mark);
    down(left);
    down(symbol);
    up(symbol);
    EXPECT_TRUE(has_key(KC_LEFT));
    expect_shift(true);
    up(left);
    expect_shift(false);
}

TEST_F(EmacsMark, NavigationReleaseDoesNotStealShiftFromHeldSymbol) {
    auto symbol = extra(S(KC_1));
    tap(mark);
    down(left);
    down(symbol);
    up(left);
    EXPECT_TRUE(has_key(KC_1));
    EXPECT_EQ(last_report.mods & MOD_BIT(KC_LSFT), MOD_BIT(KC_LSFT));
    up(symbol);
    expect_shift(false);
}

TEST_F(EmacsMark, InactiveMarkDoesNotRestoreWeakShiftClearedByAnotherKeypress) {
    auto symbol = extra(S(KC_1));
    auto a = extra(KC_A, 7);
    for (bool enabled : {false, true}) {
        SCOPED_TRACE(enabled);
        if (enabled) {
            tap(mark);
        }
        down(symbol);
        EXPECT_EQ(last_report.mods & MOD_BIT(KC_LSFT), MOD_BIT(KC_LSFT));
        reports.clear();
        down(a);
        expect_key_reports(KC_A, 0, MOD_MASK_SHIFT);
        expect_shift(false);
        up(a);
        scan(TAPPING_TERM + 1);
        expect_shift(false);
        EXPECT_EQ(get_weak_mods() & MOD_MASK_SHIFT, 0);
        up(symbol);
    }
}

TEST_F(EmacsMark, FinishedNavigationDoesNotRestoreClearedSymbolShift) {
    auto symbol = extra(S(KC_1));
    auto a = extra(KC_A, 7);
    tap(mark);
    down(left);
    down(symbol);
    up(left);
    EXPECT_EQ(last_report.mods & MOD_BIT(KC_LSFT), MOD_BIT(KC_LSFT));
    reports.clear();
    down(a);
    expect_key_reports(KC_A, 0, MOD_MASK_SHIFT);
    scan(TAPPING_TERM + 1);
    expect_shift(false);
    EXPECT_EQ(get_weak_mods() & MOD_MASK_SHIFT, 0);
    up(a);
    up(symbol);
}

TEST_F(EmacsMark, CutLineKeepsMarkButDoesNotAddItsShiftToControlX) {
    auto cut = extra(cut_line);
    tap(mark);
    down(left);
    reports.clear();
    timed_reports.clear();
    tap(cut);
    EXPECT_TRUE(set_mark_active);
    expect_key_reports(KC_END, MOD_BIT(KC_LSFT));
    expect_key_reports(KC_X, MOD_BIT(KC_LCTL), MOD_MASK_SHIFT);
    bool end_pressed = false;
    auto end_released = timed_reports.cend();
    auto cut_pressed = timed_reports.cend();
    for (auto it = timed_reports.cbegin(); it != timed_reports.cend(); ++it) {
        const auto& event = *it;
        if (std::count(std::begin(event.report.keys), std::end(event.report.keys), KC_END)) {
            end_pressed = true;
        } else if (end_pressed && end_released == timed_reports.cend()) {
            end_released = it;
        }
        if (cut_pressed == timed_reports.cend() && std::count(std::begin(event.report.keys), std::end(event.report.keys), KC_X)) {
            ASSERT_NE(end_released, timed_reports.cend());
            cut_pressed = it;
        }
    }
    ASSERT_NE(end_released, timed_reports.cend());
    ASSERT_NE(cut_pressed, timed_reports.cend());
    EXPECT_EQ(cut_pressed->time - end_released->time, 10);
    EXPECT_TRUE(has_key(KC_LEFT));
    expect_shift(true);
    up(left);
    expect_shift(false);
}

TEST_F(EmacsMark, CutLinePreservesPhysicalShiftAndHeldMouseButton) {
    auto cut = extra(cut_line);
    auto j = extra(KC_J, 7);
    auto k = extra(KC_K, 8);
    down(lshift);
    tap(mark);
    down(left);
    down(j);
    down(k, COMBO_TERM + 2);
    ASSERT_FALSE(mouse_reports.empty());
    EXPECT_EQ(mouse_reports.back().buttons, 1);
    reports.clear();
    tap(cut);
    expect_exact_key_reports(KC_END, MOD_BIT(KC_LSFT));
    expect_exact_key_reports(KC_X, MOD_BIT(KC_LCTL));
    EXPECT_EQ(get_mods(), MOD_BIT(KC_LSFT));
    EXPECT_EQ(last_report.mods, MOD_BIT(KC_LSFT) | MOD_BIT(KC_RSFT));
    EXPECT_EQ(mouse_reports.back().buttons, 1);
    tap(abort);
    EXPECT_EQ(get_mods() & MOD_BIT(KC_LSFT), MOD_BIT(KC_LSFT));
    EXPECT_EQ(mouse_reports.back().buttons, 1);
    up(left);
    up(j);
    up(k);
    EXPECT_EQ(mouse_reports.back().buttons, 0);
    up(lshift);
}

TEST_F(EmacsMark, AllFiveClickCombosKeepTheirButtonsHeldWhenMarkEnds) {
    const uint16_t chords[][2] = {{KC_J, KC_K}, {KC_K, KC_L}, {KC_J, KC_L}, {KC_M, KC_COMM}, {KC_COMM, KC_DOT}};
    for (uint8_t i = 0; i < 5; ++i) {
        SCOPED_TRACE(i);
        KeymapKey first(0, 6, 0, chords[i][0]);
        KeymapKey second(0, 7, 0, chords[i][1]);
        set_keymap({mark, abort, left, right, lshift, rshift, first, second});
        tap(mark);
        down(left);
        down(first);
        down(second, COMBO_TERM + 2);
        ASSERT_FALSE(mouse_reports.empty());
        EXPECT_EQ(mouse_reports.back().buttons, 1 << i);
        tap(abort);
        expect_shift(false);
        EXPECT_EQ(mouse_reports.back().buttons, 1 << i);
        up(first);
        EXPECT_EQ(mouse_reports.back().buttons, 1 << i);
        up(second);
        EXPECT_EQ(mouse_reports.back().buttons, 0);
        up(left);
    }
}

TEST_F(EmacsMark, ModifiedNavigationDoesNotActivateMarkShift) {
    auto gui_down = extra(G(KC_DOWN));
    tap(mark);
    down(gui_down);
    expect_key_reports(KC_DOWN, MOD_BIT(KC_LGUI), MOD_MASK_SHIFT);
    up(gui_down);
    expect_shift(false);
}

TEST_F(EmacsMark, MetaNavigationSelectsWhileMarkIsActive) {
    auto alt = extra(KC_LALT);
    for (uint16_t code : {KC_B, KC_F, KC_V}) {
        SCOPED_TRACE(code);
        KeymapKey trigger(0, 7, 0, code);
        set_keymap({mark, abort, left, right, lshift, rshift, alt, trigger});
        if (!set_mark_active) {
            tap(mark);
        }
        down(alt);
        reports.clear();
        down(trigger);
        uint8_t replacement = code == KC_B ? KC_LEFT : code == KC_F ? KC_RIGHT : KC_PGUP;
        expect_exact_key_reports(replacement, MOD_BIT(KC_LSFT) | (code == KC_V ? 0 : MOD_BIT(KC_LCTL)));
        up(trigger);
        up(alt);
        expect_shift(false);
        EXPECT_TRUE(set_mark_active);
    }
}

TEST_F(EmacsMark, DeferredOverrideWhileNavigationHeldKeepsShift) {
    auto alt = extra(KC_LALT);
    auto b = extra(KC_B, 7);
    tap(mark);
    down(right);
    down(b);
    reports.clear();
    down(alt, 600);
    expect_key_reports(KC_LEFT, MOD_BIT(KC_LCTL));
    expect_shift(true);
    up(b);
    up(alt);
    up(right);
    expect_shift(false);
}

TEST_F(EmacsMark, LayerTapAndReleaseAfterLayerChangeKeepNavigationAccounting) {
    auto layer = extra(LT(3, KC_LNG1));
    overlay(3, {KeymapKey(3, 2, 0, KC_HOME)});
    tap(mark);
    down(layer, TAPPING_TERM + 1);
    EXPECT_TRUE(layer_state_is(3));
    down(left);
    EXPECT_TRUE(has_key(KC_HOME));
    expect_shift(true);
    up(layer);
    EXPECT_FALSE(layer_state_is(3));
    up(left);
    expect_shift(false);
}

TEST_F(EmacsMark, OneShotSyntheticReleaseDoesNotLeaveShiftOrUnderflow) {
    auto oneshot = extra(OSL(4));
    overlay(4);
    tap(mark);
    tap(oneshot);
    EXPECT_TRUE(is_oneshot_layer_active());
    reports.clear();
    down(left);
    bool selected_navigation = false;
    for (const auto& report : reports) {
        if (std::count(std::begin(report.keys), std::end(report.keys), KC_LEFT)) {
            selected_navigation = true;
            EXPECT_NE(report.mods & MOD_MASK_SHIFT, 0);
        }
    }
    EXPECT_TRUE(selected_navigation);
    EXPECT_FALSE(is_oneshot_layer_active());
    EXPECT_FALSE(has_key(KC_LEFT));
    expect_shift(false);
    up(left);
    down(right);
    EXPECT_TRUE(has_key(KC_RIGHT));
    expect_shift(true);
    up(right);
    expect_shift(false);
}

TEST_F(EmacsMark, PhysicalCopyCutAndPasteEndMarkWithEitherControl) {
    for (uint16_t control_code : {KC_LCTL, KC_RCTL}) {
        for (uint16_t code : {KC_C, KC_X, KC_V}) {
            SCOPED_TRACE(control_code);
            SCOPED_TRACE(code);
            KeymapKey control(0, 6, 0, control_code);
            KeymapKey edit(0, 7, 0, code);
            set_keymap({mark, abort, left, right, lshift, rshift, control, edit});
            tap(mark);
            down(left);
            down(control);
            reports.clear();
            tap(edit);
            EXPECT_FALSE(set_mark_active);
            expect_key_reports(code, MOD_BIT(control_code), MOD_MASK_SHIFT);
            EXPECT_EQ(get_mods(), MOD_BIT(control_code));
            up(control);
            up(left);
        }
    }
}

TEST_F(EmacsMark, PlainTypingAndAltGuiShortcutsDoNotEndMark) {
    for (uint8_t mods : std::initializer_list<uint8_t>{0, MOD_LALT, MOD_LGUI, MOD_LCTL | MOD_LALT, MOD_LCTL | MOD_LGUI}) {
        for (uint16_t code : {KC_C, KC_X, KC_V}) {
            SCOPED_TRACE(mods);
            SCOPED_TRACE(code);
            KeymapKey edit(0, 6, 0, code);
            set_keymap({mark, abort, left, right, lshift, rshift, edit});
            tap(mark);
            set_mods(mods);
            tap(edit);
            EXPECT_TRUE(set_mark_active);
            clear_mods();
            send_keyboard_report();
            tap(abort);
        }
    }
}

TEST_F(EmacsMark, EncodedCopyWithAltOrGuiDoesNotEndMark) {
    for (uint16_t code : {LCA(KC_C), RCTL(RALT(KC_X)), LCTL(LGUI(KC_V))}) {
        SCOPED_TRACE(code);
        KeymapKey edit(0, 6, 0, code);
        set_keymap({mark, abort, left, right, lshift, rshift, edit});
        tap(mark);
        tap(edit);
        EXPECT_TRUE(set_mark_active);
        tap(abort);
    }
}

TEST_F(EmacsMark, OneShotControlReachesEditKeyWhenEndingHeldNavigation) {
    for (uint16_t code : {KC_C, KC_X, KC_V}) {
        SCOPED_TRACE(code);
        KeymapKey edit(0, 6, 0, code);
        set_keymap({mark, abort, left, right, lshift, rshift, edit});
        tap(mark);
        down(left);
        set_oneshot_mods(MOD_BIT(KC_RCTL));
        reports.clear();
        down(edit);
        EXPECT_FALSE(set_mark_active);
        expect_key_reports(code, MOD_BIT(KC_RCTL), MOD_MASK_SHIFT);
        EXPECT_EQ(get_oneshot_mods(), 0);
        up(edit);
        up(left);
    }
}

TEST_F(EmacsMark, MacrosIsolateAndRestoreAllPhysicalModifiers) {
    for (uint16_t macro_code : {cut_line, abort_mark}) {
        for (uint16_t modifier_code : std::initializer_list<uint16_t>{KC_LCTL, KC_RCTL, KC_LALT, KC_RALT, KC_LGUI, KC_RGUI, KC_LSFT, KC_RSFT, C(KC_LALT), C(KC_LGUI), C(KC_RSFT)}) {
            SCOPED_TRACE(macro_code);
            SCOPED_TRACE(modifier_code);
            KeymapKey modifier(0, 6, 0, modifier_code);
            KeymapKey macro(0, 7, 0, macro_code);
            set_keymap({mark, abort, left, right, lshift, rshift, modifier, macro});
            down(modifier);
            uint8_t held_mods = get_mods();
            ASSERT_NE(held_mods, 0);
            reports.clear();
            tap(macro);
            if (macro_code == cut_line) {
                expect_exact_key_reports(KC_END, MOD_BIT(KC_LSFT));
                expect_exact_key_reports(KC_X, MOD_BIT(KC_LCTL));
            } else {
                expect_key_reports(KC_ESC, 0, 0xff);
            }
            EXPECT_EQ(get_mods(), held_mods);
            EXPECT_EQ(last_report.mods, held_mods);
            up(modifier);
            EXPECT_EQ(last_report.mods, 0);
        }
    }
}

TEST_F(EmacsMark, MacrosConsumePendingOneShotModifiersWithoutApplyingThem) {
    for (uint16_t macro_code : {cut_line, abort_mark}) {
        SCOPED_TRACE(macro_code);
        KeymapKey macro(0, 6, 0, macro_code);
        KeymapKey a(0, 7, 0, KC_A);
        set_keymap({mark, abort, left, right, lshift, rshift, macro, a});
        set_oneshot_mods(MOD_MASK_CTRL | MOD_MASK_SHIFT | MOD_MASK_ALT | MOD_MASK_GUI);
        reports.clear();
        tap(macro);
        if (macro_code == cut_line) {
            expect_exact_key_reports(KC_END, MOD_BIT(KC_LSFT));
            expect_exact_key_reports(KC_X, MOD_BIT(KC_LCTL));
        } else {
            expect_key_reports(KC_ESC, 0, 0xff);
        }
        EXPECT_EQ(get_oneshot_mods(), 0);
        tap(a);
        expect_key_reports(KC_A, 0, 0xff);
    }
}

TEST_F(EmacsMark, MacrosRunAfterActiveAndDeferredOverridesAreReleased) {
    for (uint16_t macro_code : {cut_line, abort_mark}) {
        for (bool deferred : {false, true}) {
            SCOPED_TRACE(macro_code);
            SCOPED_TRACE(deferred);
            KeymapKey alt(0, 6, 0, KC_LALT);
            KeymapKey b(0, 7, 0, KC_B);
            KeymapKey macro(0, 8, 0, macro_code);
            set_keymap({mark, abort, left, right, lshift, rshift, alt, b, macro});
            down(deferred ? b : alt);
            down(deferred ? alt : b);
            reports.clear();
            tap(macro);
            if (macro_code == cut_line) {
                expect_exact_key_reports(KC_END, MOD_BIT(KC_LSFT));
                expect_exact_key_reports(KC_X, MOD_BIT(KC_LCTL));
            } else {
                expect_key_reports(KC_ESC, 0, 0xff);
            }
            scan(600);
            EXPECT_EQ(last_report.mods, MOD_BIT(KC_LALT));
            EXPECT_FALSE(has_key(KC_LEFT));
            EXPECT_FALSE(has_key(KC_B));
            EXPECT_TRUE(key_override_is_enabled());
            up(b);
            up(alt);
        }
    }
}

TEST_F(EmacsMark, MacrosDoNotEnableDisabledOverrides) {
    auto cut = extra(cut_line);
    key_override_off();
    tap(cut);
    tap(abort);
    EXPECT_FALSE(key_override_is_enabled());
    key_override_on();
}

TEST_F(EmacsMark, MacroOnOneShotLayerRunsOnceAndConsumesLayer) {
    auto oneshot = extra(OSL(4));
    overlay(4, {KeymapKey(4, 2, 0, cut_line)});
    tap(oneshot);
    reports.clear();
    down(left, 1200);
    EXPECT_FALSE(is_oneshot_layer_active());
    unsigned cut_reports = 0;
    for (const auto& report : reports) {
        cut_reports += std::count(std::begin(report.keys), std::end(report.keys), KC_X);
    }
    EXPECT_EQ(cut_reports, 1u);
    up(left);
}

TEST_F(EmacsMark, AllFourOverridesSuppressEitherAltAndPreserveShift) {
    for (uint16_t alt_code : {KC_LALT, KC_RALT}) {
        for (uint16_t code : {KC_B, KC_F, KC_V, KC_Y}) {
            for (bool shifted : {false, true}) {
                SCOPED_TRACE(alt_code);
                SCOPED_TRACE(code);
                SCOPED_TRACE(shifted);
                KeymapKey alt(0, 6, 0, alt_code);
                KeymapKey trigger(0, 7, 0, code);
                set_keymap({mark, abort, left, right, lshift, rshift, alt, trigger});
                if (shifted) {
                    down(rshift);
                }
                down(alt);
                reports.clear();
                down(trigger, 600);
                uint8_t replacement = code == KC_B ? KC_LEFT : code == KC_F ? KC_RIGHT : code == KC_V ? KC_PGUP : KC_V;
                uint8_t mods = code == KC_Y ? MOD_BIT(KC_LGUI) : code == KC_V ? 0 : MOD_BIT(KC_LCTL);
                if (shifted) {
                    mods |= MOD_BIT(KC_RSFT);
                }
                expect_exact_key_reports(replacement, mods);
                EXPECT_FALSE(has_key(code));
                up(trigger);
                EXPECT_EQ(last_report.mods, MOD_BIT(alt_code) | (shifted ? MOD_BIT(KC_RSFT) : 0));
                up(alt);
                if (shifted) {
                    up(rshift);
                }
            }
        }
    }
}

TEST_F(EmacsMark, ReleasingAltFirstNeverReregistersOriginalTrigger) {
    for (uint16_t alt_code : {KC_LALT, KC_RALT}) {
        for (uint16_t code : {KC_B, KC_F, KC_V, KC_Y}) {
            for (unsigned held_ms : {1, 600}) {
                SCOPED_TRACE(alt_code);
                SCOPED_TRACE(code);
                SCOPED_TRACE(held_ms);
                KeymapKey alt(0, 6, 0, alt_code);
                KeymapKey trigger(0, 7, 0, code);
                set_keymap({mark, abort, left, right, lshift, rshift, alt, trigger});
                down(alt);
                down(trigger, held_ms);
                reports.clear();
                up(alt, 600);
                for (const auto& report : reports) {
                    EXPECT_EQ(std::count(std::begin(report.keys), std::end(report.keys), code), 0);
                }
                EXPECT_EQ(last_report.mods, 0);
                up(trigger);
            }
        }
    }
}

TEST_F(EmacsMark, ControlAndGuiBlockOverridesIncludingTheirRelease) {
    for (uint16_t modifier_code : {KC_LCTL, KC_RCTL, KC_LGUI, KC_RGUI}) {
        for (uint16_t code : {KC_B, KC_F, KC_V, KC_Y}) {
            SCOPED_TRACE(modifier_code);
            SCOPED_TRACE(code);
            KeymapKey modifier(0, 6, 0, modifier_code);
            KeymapKey alt(0, 7, 0, KC_LALT);
            KeymapKey trigger(0, 8, 0, code);
            set_keymap({mark, abort, left, right, lshift, rshift, modifier, alt, trigger});
            down(modifier);
            down(alt);
            reports.clear();
            down(trigger);
            expect_key_reports(code, MOD_BIT(modifier_code) | MOD_BIT(KC_LALT));
            up(modifier, 600);
            EXPECT_TRUE(has_key(code));
            EXPECT_EQ(last_report.mods, MOD_BIT(KC_LALT));
            up(trigger);
            up(alt);
        }
    }
}

TEST_F(EmacsMark, AddingControlOrGuiCancelsOverrideWithoutTypingOrReactivation) {
    for (uint16_t modifier_code : {KC_LCTL, KC_RCTL, KC_LGUI, KC_RGUI}) {
        SCOPED_TRACE(modifier_code);
        KeymapKey modifier(0, 6, 0, modifier_code);
        KeymapKey alt(0, 7, 0, KC_LALT);
        KeymapKey b(0, 8, 0, KC_B);
        set_keymap({mark, abort, left, right, lshift, rshift, modifier, alt, b});
        down(alt);
        down(b);
        ASSERT_TRUE(has_key(KC_LEFT));
        reports.clear();
        down(modifier, 600);
        EXPECT_FALSE(has_key(KC_LEFT));
        up(modifier, 600);
        EXPECT_FALSE(has_key(KC_LEFT));
        for (const auto& report : reports) {
            EXPECT_EQ(std::count(std::begin(report.keys), std::end(report.keys), KC_B), 0);
        }
        up(b);
        up(alt);
    }
}

TEST_F(EmacsMark, MetaCopyEndsMarkAndDoesNotCopyWithShift) {
    for (uint16_t alt_code : {KC_LALT, KC_RALT}) {
        for (bool marking : {false, true}) {
            SCOPED_TRACE(alt_code);
            SCOPED_TRACE(marking);
            KeymapKey alt(0, 6, 0, alt_code);
            KeymapKey w(0, 7, 0, KC_W);
            set_keymap({mark, abort, left, right, lshift, rshift, alt, w});
            if (marking) {
                tap(mark);
                down(left);
            }
            down(rshift);
            down(alt);
            reports.clear();
            tap(w);
            expect_exact_key_reports(KC_C, MOD_BIT(KC_LCTL));
            expect_no_key_reports(KC_W);
            EXPECT_FALSE(set_mark_active);
            EXPECT_EQ(last_report.mods, MOD_BIT(alt_code) | MOD_BIT(KC_RSFT));
            if (marking) {
                EXPECT_TRUE(has_key(KC_LEFT));
                up(left);
            }
            up(alt);
            up(rshift);
        }
    }
}

TEST_F(EmacsMark, MetaWordCutsSelectThenCutAndEndMark) {
    for (uint16_t trigger_code : {KC_D, KC_BSPC}) {
        SCOPED_TRACE(trigger_code);
        KeymapKey alt(0, 6, 0, KC_LALT);
        KeymapKey trigger(0, 7, 0, trigger_code);
        set_keymap({mark, abort, left, right, lshift, rshift, alt, trigger});
        tap(mark);
        down(rshift);
        down(alt);
        reports.clear();
        timed_reports.clear();
        tap(trigger);
        uint8_t navigation = trigger_code == KC_D ? KC_RIGHT : KC_LEFT;
        expect_exact_key_reports(navigation, MOD_BIT(KC_LCTL) | MOD_BIT(KC_LSFT));
        expect_exact_key_reports(KC_X, MOD_BIT(KC_LCTL));
        expect_no_key_reports(trigger_code);
        EXPECT_FALSE(set_mark_active);
        EXPECT_EQ(last_report.mods, MOD_BIT(KC_LALT) | MOD_BIT(KC_RSFT));
        bool selected = false;
        auto selection_released = timed_reports.cend();
        auto cut_pressed = timed_reports.cend();
        for (auto it = timed_reports.cbegin(); it != timed_reports.cend(); ++it) {
            if (std::count(std::begin(it->report.keys), std::end(it->report.keys), navigation)) {
                selected = true;
            } else if (selected && selection_released == timed_reports.cend()) {
                selection_released = it;
            }
            if (std::count(std::begin(it->report.keys), std::end(it->report.keys), KC_X)) {
                cut_pressed = it;
                break;
            }
        }
        ASSERT_NE(selection_released, timed_reports.cend());
        ASSERT_NE(cut_pressed, timed_reports.cend());
        EXPECT_EQ(cut_pressed->time - selection_released->time, 10);
        up(alt);
        up(rshift);
    }
}

TEST_F(EmacsMark, MetaAtSelectsAWordAndKeepsMarkForFurtherNavigation) {
    auto alt = extra(KC_LALT);
    auto at = extra(KC_LBRC, 7); // JIS @, also used by the production Symbols layer.
    down(alt);
    reports.clear();
    tap(at);
    expect_exact_key_reports(KC_RIGHT, MOD_BIT(KC_LCTL) | MOD_BIT(KC_LSFT));
    expect_no_key_reports(KC_LBRC);
    EXPECT_TRUE(set_mark_active);
    up(alt);
    expect_shift(false);
    down(left);
    expect_shift(true);
    up(left);
    EXPECT_TRUE(set_mark_active);
}

TEST_F(EmacsMark, MetaDocumentNavigationConsumesInputShiftButPreservesMark) {
    for (bool marking : {false, true}) {
        for (uint16_t code : std::initializer_list<uint16_t>{KC_COMM, KC_DOT, S(KC_COMM), S(KC_DOT)}) {
            SCOPED_TRACE(marking);
            SCOPED_TRACE(code);
            KeymapKey alt(0, 6, 0, KC_LALT);
            KeymapKey trigger(0, 7, 0, code);
            set_keymap({mark, abort, left, right, lshift, rshift, alt, trigger});
            if (marking) {
                tap(mark);
            }
            down(lshift);
            down(alt);
            reports.clear();
            down(trigger, COMBO_TERM + 2);
            uint8_t replacement = QK_MODS_GET_BASIC_KEYCODE(code) == KC_COMM ? KC_HOME : KC_END;
            expect_exact_key_reports(replacement, MOD_BIT(KC_LCTL) | (marking ? MOD_BIT(KC_LSFT) : 0));
            expect_no_key_reports(QK_MODS_GET_BASIC_KEYCODE(code));
            up(trigger);
            EXPECT_EQ(last_report.mods, MOD_BIT(KC_LALT) | MOD_BIT(KC_LSFT));
            up(alt);
            up(lshift);
            EXPECT_EQ(set_mark_active, marking);
            if (marking) {
                tap(abort);
            }
        }
    }
}

TEST_F(EmacsMark, MetaNavigationStaysSelectedAcrossModifierEventsAndBareNavigation) {
    auto alt = extra(KC_LALT);
    auto f = extra(KC_F, 7);
    tap(mark);
    down(alt);
    down(f);
    down(rshift);
    up(rshift);
    EXPECT_TRUE(has_key(KC_RIGHT));
    expect_shift(true);
    down(left);
    EXPECT_FALSE(has_key(KC_RIGHT));
    expect_shift(true);
    up(f);
    EXPECT_TRUE(has_key(KC_LEFT));
    expect_shift(true);
    up(left);
    expect_shift(false);
    up(alt);
}

TEST_F(EmacsMark, MetaDocumentSelectionSurvivesBareNavigationRelease) {
    auto alt = extra(KC_LALT);
    auto comma = extra(KC_COMM, 7);
    tap(mark);
    down(left);
    down(lshift);
    down(alt);
    down(comma, COMBO_TERM + 2);
    up(left);
    EXPECT_TRUE(has_key(KC_HOME));
    expect_shift(true);
    up(comma);
    up(alt);
    up(lshift);
    expect_shift(false);
}

TEST_F(EmacsMark, MetaSelectionStopsOnAbortAndCanStartAgain) {
    auto alt = extra(KC_LALT);
    auto b = extra(KC_B, 7);
    tap(mark);
    down(alt);
    down(b);
    reports.clear();
    tap(abort);
    EXPECT_FALSE(set_mark_active);
    EXPECT_FALSE(has_key(KC_LEFT));
    expect_shift(false);
    expect_no_key_reports(KC_ESC);
    up(b);
    reports.clear();
    tap(b);
    expect_exact_key_reports(KC_LEFT, MOD_BIT(KC_LCTL));
    tap(mark);
    reports.clear();
    tap(b);
    expect_exact_key_reports(KC_LEFT, MOD_BIT(KC_LCTL) | MOD_BIT(KC_LSFT));
    up(alt);
}

TEST_F(EmacsMark, MetaMacrosRunOnceAndNeverReregisterTheirTrigger) {
    for (uint16_t alt_code : {KC_LALT, KC_RALT}) {
        for (uint16_t code : {KC_W, KC_D, KC_BSPC, KC_LBRC}) {
            SCOPED_TRACE(alt_code);
            SCOPED_TRACE(code);
            KeymapKey alt(0, 6, 0, alt_code);
            KeymapKey trigger(0, 7, 0, code);
            set_keymap({mark, abort, left, right, lshift, rshift, alt, trigger});
            down(alt);
            reports.clear();
            down(trigger, 1200);
            uint8_t output = code == KC_W ? KC_C : code == KC_LBRC ? KC_RIGHT : KC_X;
            unsigned output_reports = 0;
            for (const auto& report : reports) {
                output_reports += std::count(std::begin(report.keys), std::end(report.keys), output);
            }
            EXPECT_EQ(output_reports, 1u);
            up(alt, 600);
            up(trigger);
            expect_no_key_reports(code);
            EXPECT_EQ(last_report.mods, 0);
            if (set_mark_active) {
                tap(abort);
            }
        }
    }
}

TEST_F(EmacsMark, MetaMacrosDoNotActivateWhenAltIsPressedAfterTheLetter) {
    for (uint16_t code : {KC_W, KC_D, KC_BSPC, KC_LBRC}) {
        SCOPED_TRACE(code);
        KeymapKey alt(0, 6, 0, KC_LALT);
        KeymapKey trigger(0, 7, 0, code);
        set_keymap({mark, abort, left, right, lshift, rshift, alt, trigger});
        down(trigger);
        reports.clear();
        down(alt, 600);
        expect_no_key_reports(KC_C);
        expect_no_key_reports(KC_X);
        expect_no_key_reports(KC_RIGHT);
        EXPECT_FALSE(set_mark_active);
        up(trigger);
        up(alt);
    }
}

TEST_F(EmacsMark, MetaAdditionsRespectControlGuiAndDisabledOverrides) {
    for (uint16_t code : std::initializer_list<uint16_t>{KC_W, KC_D, KC_BSPC, KC_LBRC, S(KC_COMM), S(KC_DOT)}) {
        for (uint8_t modifier : std::initializer_list<uint8_t>{MOD_LCTL, MOD_RCTL, MOD_LGUI, MOD_RGUI}) {
            SCOPED_TRACE(code);
            SCOPED_TRACE(modifier);
            KeymapKey alt(0, 6, 0, KC_LALT);
            KeymapKey trigger(0, 7, 0, code);
            set_keymap({mark, abort, left, right, lshift, rshift, alt, trigger});
            set_mods(modifier);
            down(alt);
            reports.clear();
            tap(trigger);
            expect_key_reports(QK_MODS_GET_BASIC_KEYCODE(code), modifier | MOD_BIT(KC_LALT));
            EXPECT_FALSE(set_mark_active);
            up(alt);
            clear_mods();
            send_keyboard_report();
        }
        KeymapKey alt(0, 6, 0, KC_LALT);
        KeymapKey trigger(0, 7, 0, code);
        set_keymap({mark, abort, left, right, lshift, rshift, alt, trigger});
        key_override_off();
        down(alt);
        reports.clear();
        tap(trigger);
        expect_key_reports(QK_MODS_GET_BASIC_KEYCODE(code), MOD_BIT(KC_LALT));
        EXPECT_FALSE(set_mark_active);
        up(alt);
        key_override_on();
    }
}

TEST_F(EmacsMark, MetaWordCutReleasesPreviousOverrideAndKeepsHeldMouseButton) {
    auto alt = extra(KC_LALT);
    auto f = extra(KC_F, 7);
    auto d = extra(KC_D, 8);
    auto j = extra(KC_J, 6, 1);
    auto k = extra(KC_K, 7, 1);
    down(j);
    down(k, COMBO_TERM + 2);
    ASSERT_FALSE(mouse_reports.empty());
    ASSERT_EQ(mouse_reports.back().buttons, 1);
    tap(mark);
    down(alt);
    down(f);
    reports.clear();
    tap(d);
    expect_exact_key_reports(KC_RIGHT, MOD_BIT(KC_LCTL) | MOD_BIT(KC_LSFT));
    expect_exact_key_reports(KC_X, MOD_BIT(KC_LCTL));
    EXPECT_FALSE(set_mark_active);
    EXPECT_EQ(mouse_reports.back().buttons, 1);
    up(f);
    up(alt);
    up(j);
    up(k);
    EXPECT_EQ(mouse_reports.back().buttons, 0);
}

TEST_F(EmacsMark, MetaMacrosWorkOnOneShotLayers) {
    auto alt = extra(KC_LALT);
    auto oneshot = extra(OSL(4), 7);
    overlay(4, {KeymapKey(4, 2, 0, KC_W)});
    tap(mark);
    tap(oneshot);
    down(alt);
    reports.clear();
    tap(left);
    expect_exact_key_reports(KC_C, MOD_BIT(KC_LCTL));
    expect_no_key_reports(KC_W);
    EXPECT_FALSE(set_mark_active);
    EXPECT_FALSE(is_oneshot_layer_active());
    up(alt);
}

TEST_F(EmacsMark, MetaCustomMacroKeycodesUseTheSameModifierSafePath) {
    for (uint16_t code : {copy_region, cut_word, cut_word_backward, mark_word}) {
        SCOPED_TRACE(code);
        KeymapKey macro(0, 6, 0, code);
        set_keymap({mark, abort, left, right, lshift, rshift, macro});
        tap(mark);
        set_mods(MOD_MASK_CTRL | MOD_MASK_SHIFT | MOD_MASK_ALT | MOD_MASK_GUI);
        reports.clear();
        tap(macro);
        if (code == copy_region) {
            expect_exact_key_reports(KC_C, MOD_BIT(KC_LCTL));
        } else {
            expect_exact_key_reports(code == cut_word_backward ? KC_LEFT : KC_RIGHT, MOD_BIT(KC_LCTL) | MOD_BIT(KC_LSFT));
            if (code != mark_word) {
                expect_exact_key_reports(KC_X, MOD_BIT(KC_LCTL));
            }
        }
        EXPECT_EQ(set_mark_active, code == mark_word);
        EXPECT_EQ(get_mods(), MOD_MASK_CTRL | MOD_MASK_SHIFT | MOD_MASK_ALT | MOD_MASK_GUI);
        clear_mods();
        send_keyboard_report();
        if (set_mark_active) {
            tap(abort);
        }
    }
}

TEST_F(EmacsMark, MetaPunctuationBindingsAreReachableOnProductionSymbolsLayer) {
    set_keymap({});
    for (uint8_t layer = 0; layer < emacs_keymap_layer_count; ++layer) {
        for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
            for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
                add_key(KeymapKey(layer, col, row, pgm_read_word(&emacs_keymaps[layer][row][col])));
            }
        }
    }
    auto production_key = [this](uint16_t code, uint8_t layer) {
        auto found = std::find_if(keymap.begin(), keymap.end(), [=](const KeymapKey& candidate) {
            return candidate.layer == layer && candidate.code == code;
        });
        if (found == keymap.end()) {
            ADD_FAILURE() << "Missing production key " << code << " on layer " << +layer;
            return KeymapKey(0, 0, 0, KC_NO);
        }
        return *found;
    };
    auto symbols = production_key(LT(1, KC_SPC), 0);
    auto alt = production_key(KC_LALT, 0);
    down(symbols, TAPPING_TERM + 1);
    down(alt);
    reports.clear();
    auto less = production_key(S(KC_COMM), 1);
    down(less);
    expect_exact_key_reports(KC_HOME, MOD_BIT(KC_LCTL));
    up(less);
    auto greater = production_key(S(KC_DOT), 1);
    reports.clear();
    down(greater);
    expect_exact_key_reports(KC_END, MOD_BIT(KC_LCTL));
    up(greater);
    reports.clear();
    tap(production_key(KC_LBRC, 1));
    expect_exact_key_reports(KC_RIGHT, MOD_BIT(KC_LCTL) | MOD_BIT(KC_LSFT));
    EXPECT_TRUE(set_mark_active);
    reports.clear();
    down(less);
    expect_exact_key_reports(KC_HOME, MOD_BIT(KC_LCTL) | MOD_BIT(KC_LSFT));
    up(less);
    up(alt);
    up(symbols);
    // Use the production Abort key for cleanup rather than the fixture's map.
    layer_on(3);
    tap(production_key(abort_mark, 3));
    layer_off(3);
    EXPECT_FALSE(set_mark_active);
}
