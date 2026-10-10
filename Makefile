# The bot links no libc: syslite.h provides syscalls and string functions.
# -fno-tree-loop-distribute-patterns stops GCC turning syslite.h's memcpy/strlen
#   loops into calls to themselves (see `check`).
# build/bot.map shows what takes space and who references each symbol.
# build/convert needs -ffp-contract=off so output stays byte-identical to libopus 1.5.2.

BOTCC   := gcc -static -nostdlib
CFLAGS  := -O2 -Wall -Isrc -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fcf-protection=none
SMALL   := -Os -fno-pie -fno-stack-protector -fno-stack-clash-protection -fno-ident \
           -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -fno-tree-loop-distribute-patterns
LDFLAGS := -Wl,--gc-sections -Wl,-z,noseparate-code -Wl,-z,norelro -no-pie -Wl,--build-id=none \
           -Wl,-z,max-page-size=4096 -Wl,--sort-common \
           -Wl,-Map=build/bot.map -Wl,--cref

ASSETS          := assets
NAME            := $(shell sed -n 's/.*"mp3_file"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' config.json)
MP3             := $(ASSETS)/$(NAME).mp3
BIN             := $(ASSETS)/$(NAME).bin
BOT_HEADERS     := src/mtls.h src/dave.h src/mcrypto.h src/p256.h src/dns.h src/json.h src/stdlite.h src/syslite.h
CONVERT_HEADERS := src/mp3.h src/opusenc.h

.PHONY: all check clean

all: build/bot build/convert $(BIN) check
	@echo "Ready. Start the bot from this folder with: build/bot"
	@echo "To use a different audio file, put it in $(ASSETS)/, update config.json, and run make again."

clean:
	rm -rf build

build/bot.o: src/bot.c $(BOT_HEADERS) Makefile
	@mkdir -p build
	@$(BOTCC) $(CFLAGS) $(SMALL) -c -o $@ $< && echo "Compiled $@" || { echo "Failed to compile $@"; exit 1; }

build/bot: build/bot.o
	@$(BOTCC) -o $@ $^ $(LDFLAGS) -lgcc && echo "Linked $@" || { echo "Failed to link $@"; exit 1; }

check: build/bot
	@file build/bot | grep -q "statically linked" || { echo "ERROR: build/bot is not statically linked"; exit 1; }
	@objdump -d build/bot | awk '/^[0-9a-f]+ <.*>:$$/ { f = $$2; gsub(/[<>:]/, "", f) } /call +[0-9a-f]+ <.*>/ { t = $$NF; gsub(/[<>]/, "", t); sub(/\+.*/, "", t); if (t == f) { print "ERROR: " f " calls itself"; bad = 1 } } END { exit bad }'

build/convert: src/convert.c $(CONVERT_HEADERS) Makefile
	@mkdir -p build
	@cc $(CFLAGS) -ffp-contract=off -o $@ $< -lm && echo "Linked $@" || { echo "Failed to link $@"; exit 1; }

$(BIN): $(MP3) build/convert
	@./build/convert $< $@ || { echo "Failed to convert $@"; exit 1; }