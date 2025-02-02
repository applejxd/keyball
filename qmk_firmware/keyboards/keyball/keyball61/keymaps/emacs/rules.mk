OLED_ENABLE = yes
# LED 未使用でもマウス移動のために必要?
RGBLIGHT_ENABLE = yes

# ALT key キーの上書きのため
# https://docs.qmk.fm/features/key_overrides
KEY_OVERRIDE_ENABLE = yes

# マウスキーに使用するコンボを有効化
COMBO_ENABLE = no

#---------#
# 容量節約 #
#---------#

VIA_ENABLE = no

# see https://zenn.dev/koron/articles/98324ab760e83a

# # Link Time Optimization
# LTO_ENABLE = yes

# printf に必要なコンソール表示を無効化
CONSOLE_ENABLE = no
# Shift または Ctrl キーで括弧を入力する機能を無効化
SPACE_CADET_ENABLE = no
# Shift キーまたは GUI キーで入力する Grave (~) を無効化
GRAVE_ESC_ENABLE = no

# メディアキーを無効化
EXTRAKEY_ENABLE = no
# Magic キーコードとほぼ重複する Command 機能を無効化
COMMAND_ENABLE = no
# # Magic キーコードを無効化
MAGIC_ENABLE = no